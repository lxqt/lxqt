#ifndef FILEACTION_H
#define FILEACTION_H

#include <glib.h>
#include <string>

#include "../core/fileinfo.h"
#include "fileactioncondition.h"
#include "fileactionprofile.h"

namespace Fm {

enum class FileActionType {
    NONE,
    ACTION,
    MENU
};


enum FileActionTarget {
    FILE_ACTION_TARGET_NONE,
    FILE_ACTION_TARGET_CONTEXT = 1,
    FILE_ACTION_TARGET_LOCATION = 1 << 1,
    FILE_ACTION_TARGET_TOOLBAR = 1 << 2
};


class FileActionObject {
public:
    explicit FileActionObject();

    explicit FileActionObject(GKeyFile* kf);

    virtual ~FileActionObject();

    void setId(const char* _id) {
        id = CStrPtr{g_strdup(_id)};
    }

    static bool is_plural_exec(const char* exec);

    static std::string expand_str(const char* templ, const FileInfoList& files, bool for_display = false, std::shared_ptr<const FileInfo> first_file = nullptr);

    FileActionType type;
    CStrPtr id;
    CStrPtr name;
    CStrPtr tooltip;
    CStrPtr icon;
    CStrPtr desc;
    bool enabled;
    bool hidden;
    CStrPtr suggested_shortcut;
    std::unique_ptr<FileActionCondition> condition;

    // values cached during menu generation
    bool has_parent;
};


class FileAction: public FileActionObject {
public:

    FileAction(GKeyFile* kf);

    std::shared_ptr<FileActionProfile> match(const FileInfoList& files) const;

    int target; // bitwise or of FileActionTarget
    CStrPtr toolbar_label;

    // FIXME: currently we don't support dynamic profiles
    std::vector<std::shared_ptr<FileActionProfile>> profiles;
};


class FileActionMenu : public FileActionObject {
public:

    FileActionMenu(GKeyFile* kf);

    bool match(const FileInfoList &files) const;

    // called during menu generation
    void cache_children(const FileInfoList &files, const char** items_list);

    CStrArrayPtr items_list;

    // values cached during menu generation
    std::vector<std::shared_ptr<FileActionObject>> cached_children;
};


class FileActionItem {
public:

    static std::shared_ptr<FileActionItem> fromActionObject(std::shared_ptr<FileActionObject> action_obj, const FileInfoList &files);

    FileActionItem(std::shared_ptr<FileAction> _action, std::shared_ptr<FileActionProfile> _profile, const FileInfoList& files);

    FileActionItem(std::shared_ptr<FileActionMenu> menu, const FileInfoList& files);

    FileActionItem(std::shared_ptr<FileActionObject> _action, const FileInfoList& files);

    const std::string& get_name() const {
        return name;
    }

    const std::string& get_desc() const {
        return desc;
    }

    const std::string& get_icon() const {
        return icon;
    }

    const char* get_id() const {
        return action->id.get();
    }

    FileActionTarget get_target() const {
        if(action->type == FileActionType::ACTION) {
            return FileActionTarget(static_cast<FileAction*>(action.get())->target);
        }
        return FILE_ACTION_TARGET_CONTEXT;
    }

    bool is_menu() const {
        return (action->type == FileActionType::MENU);
    }

    bool is_action() const {
        return (action->type == FileActionType::ACTION);
    }

    bool launch(GAppLaunchContext *ctx, const FileInfoList &files, CStrPtr &output) const;

    const std::vector<std::shared_ptr<const FileActionItem>>& get_sub_items() const {
        return children;
    }

    static bool compare_items(std::shared_ptr<const FileActionItem> a, std::shared_ptr<const FileActionItem> b);
    static std::vector<std::shared_ptr<const FileActionItem>> get_actions_for_files(const FileInfoList& files);

    std::string name;
    std::string desc;
    std::string icon;
    std::shared_ptr<FileActionObject> action;
    std::shared_ptr<FileActionProfile> profile; // only used by action item
    std::vector<std::shared_ptr<const FileActionItem>> children; // only used by menu
};

typedef std::vector<std::shared_ptr<const FileActionItem>> FileActionItemList;

} // namespace Fm


#endif // FILEACTION_H
