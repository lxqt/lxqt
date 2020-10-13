namespace Fm {
	// [CCode (cheader_filename = "fm.h")]

	[Compact]
	[CCode (ref_function = "fm_path_ref", unref_function = "fm_path_unref", cname = "FmPath", cprefix = "fm_path_", cheader_filename = "fm-path.h")]
	public class Path {
		public Path();

		public unowned Path get_parent();
		public unowned string get_basename();

		// FmPathFlags get_flags();
		public bool has_prefix(Path prefix);
		public bool is_native();
		public bool is_trash();
		public bool is_trash_root();
		public bool is_virtual();
		public bool is_local();
		public bool is_xdg_menu();

		public string to_str();
		public string to_uri();
		public GLib.File to_gfile();

		public string display_name(bool human_readable);
		public string display_basename();

		public uint hash();
		public bool equal(Path* p2);

		public bool equal_str(string str, int n);
		public int depth();
	}

	[Compact]
	[CCode (ref_function = "fm_icon_ref", unref_function = "fm_icon_unref", cname = "FmIcon", cprefix = "fm_icon_", cheader_filename = "fm-icon.h")]
	public class Icon {
		public uint n_ref;
		public GLib.Icon gicon;
		public void* user_data;
	}

	[Compact]
	[CCode (ref_function = "fm_mime_type_ref", unref_function = "fm_mime_type_unref", cname = "FmMimeType", cprefix = "fm_mime_type_", cheader_filename = "fm-mime-type.h")]
	public class MimeType {
		public unowned Fm.Icon get_icon();
		public unowned string get_type();
		public unowned string get_desc();
	}

	[Compact]
	[CCode (ref_function = "fm_file_info_ref", unref_function = "fm_file_info_unref", cname = "FmFileInfo", cprefix = "fm_file_info_", cheader_filename = "fm-file-info.h")]
	public class FileInfo {
		public FileInfo ();

		public unowned Path get_path();
		public unowned string? get_name();
		public unowned string? get_disp_name();

		public void set_path(Path path);
		public void set_disp_name(string name);

		public int64 get_size();
		public unowned string? get_disp_size();

		public int64 get_blocks();

		public Posix.mode_t get_mode();

		public unowned MimeType get_mime_type();

		public bool is_dir();
		public bool is_symlink();
		public bool is_shortcut();
		public bool is_mountable();
		public bool is_image();
		public bool is_text();
		public bool is_desktop_entry();
		public bool is_unknown_type();
		public bool is_hidden;
		public bool is_executable_type();

		public unowned string? get_target();
		public unowned string? get_collate_key();
		public unowned string? get_desc();
		public unowned string? get_disp_mtime();
		public unowned time_t get_mtime();
		public unowned time_t get_atime();

		public bool can_thumbnail();
	}

	[CCode (cname = "FmAppInfo", cprefix = "fm_app_info_", cheader_filename = "fm-app-info.h")]
	namespace AppInfo {
		public bool launch(GLib.List<GLib.File> files, GLib.AppLaunchContext launch_context) throws GLib.Error;
		public bool launch_uris(GLib.List<string> uris, GLib.AppLaunchContext launch_context) throws GLib.Error;
		public bool launch_default_for_uri(string *uri, GLib.AppLaunchContext *launch_context) throws GLib.Error;
		public static unowned GLib.AppInfo create_from_commandline(string commandline, string? application_name, GLib.AppInfoCreateFlags flags) throws GLib.Error;
	}

}
