//      condition.vala
//      
//      Copyright 2011 Hong Jen Yee (PCMan) <pcman.tw@pcman.tw@gmail.com>
//      
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//      
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//      
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.
//      
//      

namespace Fm {

// FIXME: we can use getgroups() to get groups of current process
// then, call stat() and stat.st_gid to handle capabilities
// in this way, we don't have to call euidaccess

public enum FileActionCapability {
	OWNER = 0,
	READABLE = 1 << 1,
	WRITABLE = 1 << 2,
	EXECUTABLE = 1 << 3,
	LOCAL = 1 << 4
}

[Compact]
public class FileActionCondition {
	
	public FileActionCondition(KeyFile kf, string group) {
		only_show_in = Utils.key_file_get_string_list(kf, group, "OnlyShowIn");
		not_show_in = Utils.key_file_get_string_list(kf, group, "NotShowIn");
		try_exec = Utils.key_file_get_string(kf, group, "TryExec");
		show_if_registered = Utils.key_file_get_string(kf, group, "ShowIfRegistered");
		show_if_true = Utils.key_file_get_string(kf, group, "ShowIfTrue");
		show_if_running = Utils.key_file_get_string(kf, group, "ShowIfRunning");
		mime_types = Utils.key_file_get_string_list(kf, group, "MimeTypes");
		base_names = Utils.key_file_get_string_list(kf, group, "Basenames");
		match_case = Utils.key_file_get_bool(kf, group, "Matchcase");

		var selection_count_str = Utils.key_file_get_string(kf, group, "SelectionCount");
		if(selection_count_str != null) {
			switch(selection_count_str[0]) {
			case '<':
			case '>':
			case '=':
				selection_count_cmp = selection_count_str[0];
				char tmp = 0;
				const string s = "%c%d";
				selection_count_str.scanf(s, out tmp, out selection_count);
				break;
			default:
				selection_count_cmp = '>';
				selection_count = 0;
				break;
			}
		}
		else {
			selection_count_cmp = '>';
			selection_count = 0;
		}

		schemes = Utils.key_file_get_string_list(kf, group, "Schemes");
		folders = Utils.key_file_get_string_list(kf, group, "Folders");
		var caps = Utils.key_file_get_string_list(kf, group, "Capabilities");
		foreach(unowned string cap in caps) {
			stdin.printf("%s\n", cap);
		}
	}

#if 0
	private bool match_base_name_(List<FileInfo> files, string allowed_base_name) {
		// all files should match the base_name pattern.
		bool allowed = true;
		if(allowed_base_name.index_of_char('*') >= 0) {
			string allowed_base_name_ci;
			if(!match_case) {
				allowed_base_name_ci = allowed_base_name.casefold(); // FIXME: is this ok?
				allowed_base_name = allowed_base_name_ci;
			}
			var pattern= new PatternSpec(allowed_base_name);
			foreach(unowned FileInfo fi in files) {
				unowned string name = fi.get_name();
				if(match_case) {
					if(!pattern.match_string(name)) {
						allowed = false;
						break;
					}
				}
				else {
					if(!pattern.match_string(name.casefold())) {
						allowed = false;
						break;
					}
				}
			}
		}
		else {
			foreach(unowned FileInfo fi in files) {
				unowned string name = fi.get_name();
				if(match_case) {
					if(allowed_base_name != name) {
						allowed = false;
						break;
					}
				}
				else {
					if(allowed_base_name.collate(name) != 0) {
						allowed = false;
						break;
					}
				}
			}
		}
		return allowed;
	}
#endif

	private inline bool match_try_exec(List<FileInfo> files) {
		if(try_exec != null) {
			// stdout.printf("    TryExec: %s\n", try_exec);
			var exec_path = Environment.find_program_in_path(
					FileActionParameters.expand(try_exec, files));
			if(!FileUtils.test(exec_path, FileTest.IS_EXECUTABLE)) {
				return false;
			}
		}
		return true;
	}
	
	private inline bool match_show_if_registered(List<FileInfo> files) {
		if(show_if_registered != null) {
			// stdout.printf("    ShowIfRegistered: %s\n", show_if_registered);
			var service = FileActionParameters.expand(show_if_registered, files);
			// References:
			// http://people.freedesktop.org/~david/eggdbus-20091014/eggdbus-interface-org.freedesktop.DBus.html#eggdbus-method-org.freedesktop.DBus.NameHasOwner
			// glib source code: gio/tests/gdbus-names.c
			try {
				var con = Bus.get_sync(BusType.SESSION);
				var result = con.call_sync("org.freedesktop.DBus",
											"/org/freedesktop/DBus",
											"org.freedesktop.DBus",
											"NameHasOwner",
											new Variant("(s)", service),
											new VariantType("(b)"),
											DBusCallFlags.NONE, -1);
				bool name_has_owner;
				result.get("(b)", out name_has_owner);
				// stdout.printf("check if service: %s is in use: %d\n", service, (int)name_has_owner);
				if(!name_has_owner)
					return false;
			}
			catch(IOError err) {
				return false;
			}
		}
		return true;
	}
	
	private inline bool match_show_if_true(List<FileInfo> files) {
		if(show_if_true != null) {
			var cmd = FileActionParameters.expand(show_if_true, files);
			int exit_status;
			// FIXME: Process.spawn cannot handle shell commands. Use Posix.system() instead.
			//if(!Process.spawn_command_line_sync(cmd, null, null, out exit_status)
			//	|| exit_status != 0)
			//	return false;
			exit_status = Posix.system(cmd);
			if(exit_status != 0)
				return false;
		}
		return true;
	}

	private inline bool match_show_if_running(List<FileInfo> files) {
		if(show_if_running != null) {
			var process_name = FileActionParameters.expand(show_if_running, files);
			var pgrep = Environment.find_program_in_path("pgrep");
			bool running = false;
			// pgrep is not fully portable, but we don't have better options here
			if(pgrep != null) {
				int exit_status;
				if(Process.spawn_command_line_sync(@"$pgrep -x '$process_name'", 
												null, null, out exit_status)) {
					if(exit_status == 0)
						running = true;
				}
			}
			if(!running)
				return false;
		}
		return true;
	}

	private inline bool match_mime_type(List<FileInfo> files, string type, bool negated) {
		// stdout.printf("match_mime_type: %s, neg: %d\n", type, (int)negated);

		if(type == "all/all" || type == "*") {
			return negated ? false : true;
		}
		else if(type == "all/allfiles") {
			// see if all fileinfos are files
			if(negated) { // all fileinfos should not be files
				foreach(unowned FileInfo fi in files) {
					if(!fi.is_dir()) { // at least 1 of the fileinfos is a file.
						return false;
					}
				}
			}
			else { // all fileinfos should be files
				foreach(unowned FileInfo fi in files) {
					if(fi.is_dir()) { // at least 1 of the fileinfos is a file.
						return false;
					}
				}
			}
		}
		else if(type.has_suffix("/*")) {
			// check if all are subtypes of allowed_type
			var prefix = type[0:-1];
			if(negated) { // all files should not have the prefix
				foreach(unowned FileInfo fi in files) {
					if(fi.get_mime_type().get_type().has_prefix(prefix)) {
						return false;
					}
				}
			}
			else { // all files should have the prefix
				foreach(unowned FileInfo fi in files) {
					if(!fi.get_mime_type().get_type().has_prefix(prefix)) {
						return false;
					}
				}
			}
		}
		else {
			if(negated) { // all files should not be of the type
				foreach(unowned FileInfo fi in files) {
					if(fi.get_mime_type().get_type() == type) {
					// if(ContentType.is_a(type, fi.get_mime_type().get_type())) {
						return false;
					}
				}
			}
			else { // all files should be of the type
				foreach(unowned FileInfo fi in files) {
					// stdout.printf("get_type: %s, type: %s\n", fi.get_mime_type().get_type(), type);
					if(fi.get_mime_type().get_type() != type) {
					// if(!ContentType.is_a(type, fi.get_mime_type().get_type())) {
						return false;
					}
				}
			}
		}
		return true;
	}

	private inline bool match_mime_types(List<FileInfo> files) {
		if(mime_types != null) {
			bool allowed = false;
			// FIXME: this is inefficient, but easier to implement
			// check if all of the mime_types are allowed
			foreach(unowned string allowed_type in mime_types) {
				unowned string type;
				bool negated;
				if(allowed_type[0] == '!') {
					type = (string)((uint8*)allowed_type + 1);
					negated = true;
				}
				else {
					type = allowed_type;
					negated = false;
				}

				if(negated) { // negated mime_type rules are ANDed
					bool type_is_allowed = match_mime_type(files, type, negated);
					if(!type_is_allowed) // so any mismatch is not allowed
						return false;
				}
				else { // other mime_type rules are ORed
					// matching any one of the mime_type is enough
					if(!allowed) { // if no rule is matched yet
						allowed = match_mime_type(files, type, false);
					}
				}
			}
			return allowed;
		}
		return true;
	}

	private inline bool match_base_name(List<FileInfo> files, string base_name, bool negated) {
		// see if all files has the base_name
		// FIXME: this is inefficient, some optimization is needed later
		PatternSpec pattern;
		if(match_case)
			pattern= new PatternSpec(base_name);
		else
			pattern= new PatternSpec(base_name.casefold()); // FIXME: is this correct?
		foreach(unowned FileInfo fi in files) {
			var name = fi.get_name();
			if(match_case) {
				if(pattern.match_string(name)) {
					// at least 1 file has the base_name
					if(negated)
						return false;
				}
				else {
					// at least 1 file does not has the scheme
					if(!negated)
						return false;
				}
			}
			else {
				if(pattern.match_string(name.casefold())) {
					// at least 1 file has the base_name
					if(negated)
						return false;
				}
				else {
					// at least 1 file does not has the scheme
					if(!negated)
						return false;
				}
			}
		}
		return true;
	}

	private inline bool match_base_names(List<FileInfo> files) {
		if(base_names != null) {
			bool allowed = false;
			// FIXME: this is inefficient, but easier to implement
			// check if all of the base_names are allowed
			foreach(unowned string allowed_name in base_names) {
				unowned string name;
				bool negated;
				if(allowed_name[0] == '!') {
					name = (string)((uint8*)allowed_name + 1);
					negated = true;
				}
				else {
					name = allowed_name;
					negated = false;
				}

				if(negated) { // negated base_name rules are ANDed
					bool name_is_allowed = match_base_name(files, name, negated);
					if(!name_is_allowed) // so any mismatch is not allowed
						return false;
				}
				else { // other base_name rules are ORed
					// matching any one of the base_name is enough
					if(!allowed) { // if no rule is matched yet
						allowed = match_base_name(files, name, false);
					}
				}
			}
			return allowed;
		}
		return true;
	}

	private static bool match_scheme(List<FileInfo> files, string scheme, bool negated) {
		// FIXME: this is inefficient, some optimization is needed later
		// see if all files has the scheme
		foreach(unowned FileInfo fi in files) {
			var uri = fi.get_path().to_uri();
			if(Uri.parse_scheme(uri) == scheme) {
				// at least 1 file has the scheme
				if(negated)
					return false;
			}
			else {
				// at least 1 file does not has the scheme
				if(!negated)
					return false;
			}
		}
		return true;
	}

	private inline bool match_schemes(List<FileInfo> files) {
		if(schemes != null) {
			bool allowed = false;
			// FIXME: this is inefficient, but easier to implement
			// check if all of the schemes are allowed
			foreach(unowned string allowed_scheme in schemes) {
				unowned string scheme;
				bool negated;
				if(allowed_scheme[0] == '!') {
					scheme = (string)((uint8*)allowed_scheme + 1);
					negated = true;
				}
				else {
					scheme = allowed_scheme;
					negated = false;
				}

				if(negated) { // negated scheme rules are ANDed
					bool scheme_is_allowed = match_scheme(files, scheme, negated);
					if(!scheme_is_allowed) // so any mismatch is not allowed
						return false;
				}
				else { // other scheme rules are ORed
					// matching any one of the scheme is enough
					if(!allowed) { // if no rule is matched yet
						allowed = match_scheme(files, scheme, false);
					}
				}
			}
			return allowed;
		}
		return true;
	}

	private static bool match_folder(List<FileInfo> files, string folder, bool negated) {
		// trailing /* should always be implied.
		// FIXME: this is inefficient, some optimization is needed later
		PatternSpec pattern;
		if(folder.has_suffix("/*"))
			pattern = new PatternSpec(folder);
		else
			pattern = new PatternSpec(@"$folder/*");
		foreach(unowned FileInfo fi in files) {
			var dirname = fi.get_path().get_parent().to_str();
			if(pattern.match_string(dirname)) { // at least 1 file is in the folder
				if(negated)
					return false;
			}
			else {
				if(!negated)
					return false;
			}
		}
		return true;
	}

	private inline bool match_folders(List<FileInfo> files) {
		if(folders != null) {
			bool allowed = false;
			// FIXME: this is inefficient, but easier to implement
			// check if all of the schemes are allowed
			foreach(unowned string allowed_folder in folders) {
				unowned string folder;
				bool negated;
				if(allowed_folder[0] == '!') {
					folder = (string)((uint8*)allowed_folder + 1);
					negated = true;
				}
				else {
					folder = allowed_folder;
					negated = false;
				}

				if(negated) { // negated folder rules are ANDed
					bool folder_is_allowed = match_folder(files, folder, negated);
					if(!folder_is_allowed) // so any mismatch is not allowed
						return false;
				}
				else { // other folder rules are ORed
					// matching any one of the folder is enough
					if(!allowed) { // if no rule is matched yet
						allowed = match_folder(files, folder, false);
					}
				}
			}
			return allowed;
		}
		return true;
	}

	private inline bool match_selection_count(List<FileInfo> files) {
		uint n_files = files.length();
		switch(selection_count_cmp) {
		case '<':
			if(n_files >= selection_count)
				return false;
			break;
		case '=':
			if(n_files != selection_count)
				return false;
			break;
		case '>':
			if(n_files <= selection_count)
				return false;
			break;
		}
		return true;
	}

	private inline bool match_capabilities(List<FileInfo> files) {
		// TODO
		return true;
	}

	public bool match(List<FileInfo> files) {
		// all of the condition are combined with AND
		// So, if any one of the conditions is not matched, we quit.

		// TODO: OnlyShowIn, NotShowIn
		if(!match_try_exec(files))
			return false;

		if(!match_mime_types(files))
			return false;
		if(!match_base_names(files))
			return false;
		if(!match_selection_count(files))
			return false;
		if(!match_schemes(files))
			return false;
		if(!match_folders(files))
			return false;
		// TODO: Capabilities
		// currently, due to limitations of Fm.FileInfo, this cannot
		// be implemanted correctly.
		if(!match_capabilities(files))
			return false;

		if(!match_show_if_registered(files))
			return false;
		if(!match_show_if_true(files))
			return false;
		if(!match_show_if_running(files))
			return false;

		return true;
	}

	public string[]? only_show_in;
	public string[]? not_show_in;
	public string? try_exec;
	public string? show_if_registered;
	public string? show_if_true;
	public string? show_if_running;
	public string[]? mime_types;
	public string[]? base_names;
	public bool match_case;
	public char selection_count_cmp;
	public int selection_count;
	public string[]? schemes;
	public string[]? folders;
	public FileActionCapability capabilities;
}

}
