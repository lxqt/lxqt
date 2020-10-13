#include "fileaction.h"
#include <unordered_map>
#include <QDebug>

using namespace std;

namespace Fm {

static const char* desktop_env = nullptr; // current desktop environment
static bool actions_loaded = false; // all actions are loaded?
static unordered_map<const char*, shared_ptr<FileActionObject>, CStrHash, CStrEqual> all_actions; // cache all loaded actions

FileActionObject::FileActionObject() {
}

FileActionObject::FileActionObject(GKeyFile* kf) {
    name = CStrPtr{g_key_file_get_locale_string(kf, "Desktop Entry", "Name", nullptr, nullptr)};
    tooltip = CStrPtr{g_key_file_get_locale_string(kf, "Desktop Entry", "Tooltip", nullptr, nullptr)};
    icon = CStrPtr{g_key_file_get_locale_string(kf, "Desktop Entry", "Icon", nullptr, nullptr)};
    desc = CStrPtr{g_key_file_get_locale_string(kf, "Desktop Entry", "Description", nullptr, nullptr)};
    GErrorPtr err;
    enabled = g_key_file_get_boolean(kf, "Desktop Entry", "Enabled", &err);
    if(err) { // key not found, default to true
        err.reset();
        enabled = true;
    }
    hidden = g_key_file_get_boolean(kf, "Desktop Entry", "Hidden", nullptr);
    suggested_shortcut = CStrPtr{g_key_file_get_string(kf, "Desktop Entry", "SuggestedShortcut", nullptr)};

    condition = unique_ptr<FileActionCondition> {new FileActionCondition(kf, "Desktop Entry")};

    has_parent = false;
}

FileActionObject::~FileActionObject() {
}

//static
bool FileActionObject::is_plural_exec(const char* exec) {
    if(!exec) {
        return false;
    }
    // the first relevent code encountered in Exec parameter
    // determines whether the command accepts singular or plural forms
    for(int i = 0; exec[i]; ++i) {
        char ch = exec[i];
        if(ch == '%') {
            ++i;
            ch = exec[i];
            switch(ch) {
            case 'B':
            case 'D':
            case 'F':
            case 'M':
            case 'O':
            case 'U':
            case 'W':
            case 'X':
                return true;	// plural
            case 'b':
            case 'd':
            case 'f':
            case 'm':
            case 'o':
            case 'u':
            case 'w':
            case 'x':
                return false;	// singular
            default:
                // irrelevent code, skip
                break;
            }
        }
    }
    return false; // singular form by default
}

std::string FileActionObject::expand_str(const char* templ, const FileInfoList& files, bool for_display, std::shared_ptr<const FileInfo> first_file) {
    if(!templ) {
        return string{};
    }
    string result;

    if(!first_file) {
        first_file = files.front();
    }

    for(int i = 0; templ[i]; ++i) {
        char ch = templ[i];
        if(ch == '%') {
            ++i;
            ch = templ[i];
            switch(ch) {
            case 'b':	// (first) basename
                if(for_display) {
                    result += first_file->name();
                }
                else {
                    CStrPtr quoted{g_shell_quote(first_file->name().c_str())};
                    result += quoted.get();
                }
                break;
            case 'B':	// space-separated list of basenames
                for(auto& fi : files) {
                    if(for_display) {
                        result += fi->name();
                    }
                    else {
                        CStrPtr quoted{g_shell_quote(fi->name().c_str())};
                        result += quoted.get();
                    }
                    result += ' ';
                }
                if(result[result.length() - 1] == ' ') { // remove trailing space
                    result.erase(result.length() - 1);
                }
                break;
            case 'c':	// count of selected items
                result += to_string(files.size());
                break;
            case 'd': {	// (first) base directory
                // FIXME: should the base dir be a URI?
                auto base_dir = first_file->dirPath();
                auto str = base_dir.toString();
                if(for_display) {
                    // FIXME: str = Filename.display_name(str);
                }
                CStrPtr quoted{g_shell_quote(str.get())};
                result += quoted.get();
                break;
            }
            case 'D':	// space-separated list of base directory of each selected items
                for(auto& fi : files) {
                    auto base_dir = fi->dirPath();
                    auto str = base_dir.toString();
                    if(for_display) {
                        // str = Filename.display_name(str);
                    }
                    CStrPtr quoted{g_shell_quote(str.get())};
                    result += quoted.get();
                    result += ' ';
                }
                if(result[result.length() - 1] == ' ') { // remove trailing space
                    result.erase(result.length() - 1);
                }
                break;
            case 'f': {	// (first) file name
                auto filename = first_file->path().toString();
                if(for_display) {
                    // filename = Filename.display_name(filename);
                }
                CStrPtr quoted{g_shell_quote(filename.get())};
                result += quoted.get();
                break;
            }
            case 'F':	// space-separated list of selected file names
                for(auto& fi : files) {
                    auto filename = fi->path().toString();
                    if(for_display) {
                        // filename = Filename.display_name(filename);
                    }
                    CStrPtr quoted{g_shell_quote(filename.get())};
                    result += quoted.get();
                    result += ' ';
                }
                if(result[result.length() - 1] == ' ') { // remove trailing space
                    result.erase(result.length() - 1);
                }
                break;
            case 'h':	// hostname of the (first) URI
                // FIXME: how to support this correctly?
                // FIXME: currently we pass g_get_host_name()
                result += g_get_host_name();
                break;
            case 'm':	// mimetype of the (first) selected item
                result += first_file->mimeType()->name();
                break;
            case 'M':	// space-separated list of the mimetypes of the selected items
                for(auto& fi : files) {
                    result += fi->mimeType()->name();
                    result += ' ';
                }
                break;
            case 'n':	// username of the (first) URI
                // FIXME: how to support this correctly?
                result += g_get_user_name();
                break;
            case 'o':	// no-op operator which forces a singular form of execution when specified as first parameter,
            case 'O':	// no-op operator which forces a plural form of execution when specified as first parameter,
                break;
            case 'p':	// port number of the (first) URI
                // FIXME: how to support this correctly?
                // result.append("0");
                break;
            case 's':	// scheme of the (first) URI
                result += first_file->path().uriScheme().get();
                break;
            case 'u':	// (first) URI
                result += first_file->path().uri().get();
                break;
            case 'U':	// space-separated list of selected URIs
                for(auto& fi : files) {
                    result += fi->path().uri().get();
                    result += ' ';
                }
                if(result[result.length() - 1] == ' ') { // remove trailing space
                    result.erase(result.length() - 1);
                }
                break;
            case 'w': {	// (first) basename without the extension
                auto basename = first_file->name();
                int pos = basename.rfind('.');
                // FIXME: handle non-UTF8 filenames
                if(pos != -1) {
                    basename.erase(pos, string::npos);
                }
                CStrPtr quoted{g_shell_quote(basename.c_str())};
                result += quoted.get();
                break;
            }
            case 'W':	// space-separated list of basenames without their extension
                for(auto& fi : files) {
                    auto basename = fi->name();
                    int pos = basename.rfind('.');
                    // FIXME: for_display ? Shell.quote(str) : str);
                    if(pos != -1) {
                        basename.erase(pos, string::npos);
                    }
                    CStrPtr quoted{g_shell_quote(basename.c_str())};
                    result += quoted.get();
                    result += ' ';
                }
                if(result[result.length() - 1] == ' ') { // remove trailing space
                    result.erase(result.length() - 1);
                }
                break;
            case 'x': {	// (first) extension
                auto basename = first_file->name();
                int pos = basename.rfind('.');
                const char* ext = "";
                if(pos >= 0) {
                    ext = basename.c_str() + pos + 1;
                }
                CStrPtr quoted{g_shell_quote(ext)};
                result += quoted.get();
                break;
            }
            case 'X':	// space-separated list of extensions
                for(auto& fi : files) {
                    auto basename = fi->name();
                    int pos = basename.rfind('.');
                    const char* ext = "";
                    if(pos >= 0) {
                        ext = basename.c_str() + pos + 1;
                    }
                    CStrPtr quoted{g_shell_quote(ext)};
                    result += quoted.get();
                    result += ' ';
                }
                if(result[result.length() - 1] == ' ') { // remove trailing space
                    result.erase(result.length() - 1);
                }
                break;
            case '%':	// the % character
                result += '%';
                break;
            case '\0':
                break;
            }
        }
        else {
            result += ch;
        }
    }
    return result;
}

FileAction::FileAction(GKeyFile* kf): FileActionObject{kf}, target{FILE_ACTION_TARGET_CONTEXT} {
    type = FileActionType::ACTION;

    GErrorPtr err;
    if(g_key_file_get_boolean(kf, "Desktop Entry", "TargetContext", &err)) { // default to true
        target |= FILE_ACTION_TARGET_CONTEXT;
    }
    else if(!err) { // error means the key is abscent
        target &= ~FILE_ACTION_TARGET_CONTEXT;
    }
    if(g_key_file_get_boolean(kf, "Desktop Entry", "TargetLocation", nullptr)) {
        target |= FILE_ACTION_TARGET_LOCATION;
    }
    if(g_key_file_get_boolean(kf, "Desktop Entry", "TargetToolbar", nullptr)) {
        target |= FILE_ACTION_TARGET_TOOLBAR;
    }
    toolbar_label = CStrPtr{g_key_file_get_locale_string(kf, "Desktop Entry", "ToolbarLabel", nullptr, nullptr)};

    auto profile_names = CStrArrayPtr{g_key_file_get_string_list(kf, "Desktop Entry", "Profiles", nullptr, nullptr)};
    if(profile_names != nullptr) {
        for(auto profile_name = profile_names.get(); *profile_name; ++profile_name) {
            // stdout.printf("%s", profile);
            profiles.push_back(make_shared<FileActionProfile>(kf, *profile_name));
        }
    }
}

std::shared_ptr<FileActionProfile> FileAction::match(const FileInfoList& files) const {
    //qDebug() << "FileAction.match: " << id.get();
    if(hidden || !enabled) {
        return nullptr;
    }

    if(!condition->match(files)) {
        return nullptr;
    }
    for(const auto& profile : profiles) {
        if(profile->match(files)) {
            //qDebug() << "  profile matched!\n\n";
            return profile;
        }
    }
    // stdout.printf("\n");
    return nullptr;
}

FileActionMenu::FileActionMenu(GKeyFile* kf): FileActionObject{kf} {
    type = FileActionType::MENU;
    items_list = CStrArrayPtr{g_key_file_get_string_list(kf, "Desktop Entry", "ItemsList", nullptr, nullptr)};
}

bool FileActionMenu::match(const FileInfoList& files) const {
    // stdout.printf("FileActionMenu.match: %s\n", id);
    if(hidden || !enabled) {
        return false;
    }
    if(!condition->match(files)) {
        return false;
    }
    // stdout.printf("menu matched!: %s\n\n", id);
    return true;
}

void FileActionMenu::cache_children(const FileInfoList& files, const char** items_list) {
    for(; *items_list; ++items_list) {
        const char* item_id_prefix = *items_list;
        size_t len = strlen(item_id_prefix);
        if(item_id_prefix[0] == '[' && item_id_prefix[len - 1] == ']') {
            // runtime dynamic item list
            char* output;
            int exit_status;
            string prefix{item_id_prefix + 1, len - 2}; // skip [ and ]
            auto command = expand_str(prefix.c_str(), files);
            if(g_spawn_command_line_sync(command.c_str(), &output, nullptr, &exit_status, nullptr) && exit_status == 0) {
                CStrArrayPtr item_ids{g_strsplit(output, ";", -1)};
                g_free(output);
                cache_children(files, (const char**)item_ids.get());
            }
        }
        else if(strcmp(item_id_prefix, "SEPARATOR") == 0) {
            // separator item
            cached_children.push_back(nullptr);
        }
        else {
            CStrPtr item_id{g_strconcat(item_id_prefix, ".desktop", nullptr)};
            auto it = all_actions.find(item_id.get());
            if(it != all_actions.end()) {
                auto child_action = it->second;
                child_action->has_parent = true;
                cached_children.push_back(child_action);
                // stdout.printf("add child: %s to menu: %s\n", item_id, id);
            }
        }
    }
}

std::shared_ptr<FileActionItem> FileActionItem::fromActionObject(std::shared_ptr<FileActionObject> action_obj, const FileInfoList& files) {
    std::shared_ptr<FileActionItem> item;
    if(action_obj->type == FileActionType::MENU) {
        auto menu = static_pointer_cast<FileActionMenu>(action_obj);
        if(menu->match(files)) {
            item = make_shared<FileActionItem>(menu, files);
            // eliminate empty menus
            if(item->children.empty()) {
                item = nullptr;
            }
        }
    }
    else {
        // handle profiles here
        auto action = static_pointer_cast<FileAction>(action_obj);
        auto profile = action->match(files);
        if(profile) {
            item = make_shared<FileActionItem>(action, profile, files);
        }
    }
    return item;
}

FileActionItem::FileActionItem(std::shared_ptr<FileAction> _action, std::shared_ptr<FileActionProfile> _profile, const FileInfoList& files):
    FileActionItem{static_pointer_cast<FileActionObject>(_action), files} {
    profile = _profile;
}

FileActionItem::FileActionItem(std::shared_ptr<FileActionMenu> menu, const FileInfoList& files):
    FileActionItem{static_pointer_cast<FileActionObject>(menu), files} {
    for(auto& action_obj : menu->cached_children) {
        if(action_obj == nullptr) { // separator
            children.push_back(nullptr);
        }
        else { // action item or menu
            auto subitem = fromActionObject(action_obj, files);
            if(subitem != nullptr) {
                children.push_back(subitem);
            }
        }
    }
}

FileActionItem::FileActionItem(std::shared_ptr<FileActionObject> _action, const FileInfoList& files) {
    action = std::move(_action);
    name = FileActionObject::expand_str(action->name.get(), files, true);
    desc = FileActionObject::expand_str(action->desc.get(), files, true);
    icon = FileActionObject::expand_str(action->icon.get(), files, false);
}

bool FileActionItem::launch(GAppLaunchContext* ctx, const FileInfoList& files, CStrPtr& output) const {
    if(action->type == FileActionType::ACTION) {
        if(profile != nullptr) {
            profile->launch(ctx, files, output);
        }
        return true;
    }
    return false;
}

static void load_actions_from_dir(const char* dirname, const char* id_prefix) {
    //qDebug() << "loading from: " << dirname << endl;
    auto dir = g_dir_open(dirname, 0, nullptr);
    if(dir != nullptr) {
        for(;;) {
            const char* name = g_dir_read_name(dir);
            if(name == nullptr) {
                break;
            }
            // found a file in file-manager/actions dir, get its full path
            CStrPtr full_path{g_build_filename(dirname, name, nullptr)};
            // stdout.printf("\nfound %s\n", full_path);

            // see if it's a sub dir
            if(g_file_test(full_path.get(), G_FILE_TEST_IS_DIR)) {
                // load sub dirs recursively
                CStrPtr new_id_prefix;
                if(id_prefix) {
                    new_id_prefix = CStrPtr{g_strconcat(id_prefix, "-", name, nullptr)};
                }
                load_actions_from_dir(full_path.get(), id_prefix ? new_id_prefix.get() : name);
            }
            else if(g_str_has_suffix(name, ".desktop")) {
                CStrPtr new_id_prefix;
                if(id_prefix) {
                    new_id_prefix = CStrPtr{g_strconcat(id_prefix, "-", name, nullptr)};
                }
                const char* id = id_prefix ? new_id_prefix.get() : name;
                // ensure that it's not already in the cache
                if(all_actions.find(id) == all_actions.cend()) {
                    auto kf = g_key_file_new();
                    if(g_key_file_load_from_file(kf, full_path.get(), G_KEY_FILE_NONE, nullptr)) {
                        auto type = CStrPtr{g_key_file_get_string(kf, "Desktop Entry", "Type", nullptr)};
                        if(!type) {
                            continue;
                        }
                        std::shared_ptr<FileActionObject> action;
                        if(strcmp(type.get(), "Action") == 0) {
                            action = static_pointer_cast<FileActionObject>(make_shared<FileAction>(kf));
                            // stdout.printf("load action: %s\n", id);
                        }
                        else if(strcmp(type.get(), "Menu") == 0) {
                            action = static_pointer_cast<FileActionObject>(make_shared<FileActionMenu>(kf));
                            // stdout.printf("load menu: %s\n", id);
                        }
                        else {
                            continue;
                        }
                        action->setId(id);
                        all_actions.insert(make_pair(action->id.get(), action)); // add the id/action pair to hash table
                        // stdout.printf("add to cache %s\n", id);
                    }
                    g_key_file_free(kf);
                }
                else {
                    // stdout.printf("cache found for action: %s\n", id);
                }
            }
        }
        g_dir_close(dir);
    }
}

void file_actions_set_desktop_env(const char* env) {
    desktop_env = env;
}

static void load_all_actions() {
    all_actions.clear();
    auto dirs = g_get_system_data_dirs();
    for(auto dir = dirs; *dir; ++dir) {
        CStrPtr dir_path{g_build_filename(*dir, "file-manager/actions", nullptr)};
        load_actions_from_dir(dir_path.get(), nullptr);
    }
    CStrPtr dir_path{g_build_filename(g_get_user_data_dir(), "file-manager/actions", nullptr)};
    load_actions_from_dir(dir_path.get(), nullptr);
    actions_loaded = true;
}

bool FileActionItem::compare_items(std::shared_ptr<const FileActionItem> a, std::shared_ptr<const FileActionItem> b)
{
    // first get the list of level-zero item names (http://www.nautilus-actions.org/?q=node/377)
    static QStringList itemNamesList;
    static bool level_zero_checked = false;
    if(!level_zero_checked) {
        level_zero_checked = true;
        auto level_zero = CStrPtr{g_build_filename(g_get_user_data_dir(),
                                                   "file-manager/actions/level-zero.directory", nullptr)};
        if(g_file_test(level_zero.get(), G_FILE_TEST_IS_REGULAR)) {
            GKeyFile* kf = g_key_file_new();
            if(g_key_file_load_from_file(kf, level_zero.get(), G_KEY_FILE_NONE, nullptr)) {
                auto itemsList = CStrArrayPtr{g_key_file_get_string_list(kf,
                                                                         "Desktop Entry",
                                                                         "ItemsList", nullptr, nullptr)};
                if(itemsList) {
                    for(uint i = 0; i < g_strv_length(itemsList.get()); ++i) {
                        CStrPtr desktop_file_name{g_strconcat(itemsList.get()[i], ".desktop", nullptr)};
                        auto desktop_file = CStrPtr{g_build_filename(g_get_user_data_dir(),
                                                                     "file-manager/actions",
                                                                     desktop_file_name.get(), nullptr)};
                        GKeyFile* desktop_file_key = g_key_file_new();
                        if(g_key_file_load_from_file(desktop_file_key, desktop_file.get(), G_KEY_FILE_NONE, nullptr)) {
                            auto actionName = CStrPtr{g_key_file_get_locale_string(desktop_file_key,
                                                                                   "Desktop Entry",
                                                                                   "Name",
                                                                                   nullptr, nullptr)};
                            if(actionName) {
                                itemNamesList << QString::fromUtf8(actionName.get());
                            }
                        }
                        g_key_file_free(desktop_file_key);
                    }
                }
            }
            g_key_file_free(kf);
        }
    }
    if(!itemNamesList.isEmpty()) {
        int first = itemNamesList.indexOf(QString::fromStdString(a->get_name()));
        int second = itemNamesList.indexOf(QString::fromStdString(b->get_name()));
        if(first > -1) {
            if(second > -1) {
                return (first < second);
            }
            else {
                return true; // list items have priority
            }
        }
        else if(second > -1) {
            return false;
        }
    }
    return (a->get_name().compare(b->get_name()) < 0);
}

FileActionItemList FileActionItem::get_actions_for_files(const FileInfoList& files) {
    if(!actions_loaded) {
        load_all_actions();
    }

    // Iterate over all actions to establish association between parent menu
    // and children actions, and to find out toplevel ones which are not
    // attached to any parent menu
    for(auto& item : all_actions) {
        auto& action_obj = item.second;
        // stdout.printf("id = %s\n", action_obj.id);
        if(action_obj->type == FileActionType::MENU) { // this is a menu
            auto menu = static_pointer_cast<FileActionMenu>(action_obj);
            // stdout.printf("menu: %s\n", menu.name);
            // associate child items with menus
            menu->cache_children(files, (const char**)menu->items_list.get());
        }
    }

    // Output the menus
    FileActionItemList items;

    for(auto& item : all_actions) {
        auto& action_obj = item.second;
        // only output toplevel items here
        if(action_obj->has_parent == false) { // this is a toplevel item
            auto item = FileActionItem::fromActionObject(action_obj, files);
            if(item != nullptr) {
                items.push_back(item);
            }
        }
    }

    // cleanup temporary data cached during menu generation
    for(auto& item : all_actions) {
        auto& action_obj = item.second;
        action_obj->has_parent = false;
        if(action_obj->type == FileActionType::MENU) {
            auto menu = static_pointer_cast<FileActionMenu>(action_obj);
            menu->cached_children.clear();
        }
    }

    std::sort(items.begin(), items.end(), compare_items);
    return items;
}

} // namespace Fm
