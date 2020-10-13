//      profile.vala
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

public enum FileActionExecMode {
	NORMAL,
	TERMINAL,
	EMBEDDED,
	DISPLAY_OUTPUT
}

[Compact]
public class FileActionProfile {

	public FileActionProfile(KeyFile kf, string profile_name) {
		id = profile_name;
		string group_name = @"X-Action-Profile $profile_name";
		name = Utils.key_file_get_string(kf, group_name, "Name");
		exec = Utils.key_file_get_string(kf, group_name, "Exec");
		// stdout.printf("id: %s, Exec: %s\n", id, exec);

		path = Utils.key_file_get_string(kf, group_name, "Path");
		var s = Utils.key_file_get_string(kf, group_name, "ExecutionMode");
		if(s == "Normal")
			exec_mode = FileActionExecMode.NORMAL;
		else if( s == "Terminal")
			exec_mode = FileActionExecMode.TERMINAL;
		else if(s == "Embedded")
			exec_mode = FileActionExecMode.EMBEDDED;
		else if( s == "DisplayOutput")
			exec_mode = FileActionExecMode.DISPLAY_OUTPUT;
		else
			exec_mode = FileActionExecMode.NORMAL;

		startup_notify = Utils.key_file_get_bool(kf, group_name, "StartupNotify");
		startup_wm_class = Utils.key_file_get_string(kf, group_name, "StartupWMClass");
		exec_as = Utils.key_file_get_string(kf, group_name, "ExecuteAs");

		condition = new FileActionCondition(kf, group_name);
	}

	private bool launch_once(AppLaunchContext ctx, FileInfo? first_file, List<FileInfo> files, out string? output) {
		if(exec == null)
			return false;
		var exec_cmd = FileActionParameters.expand(exec, files, false, first_file);
		stdout.printf("Profile: %s\nlaunch command: %s\n\n", id, exec_cmd);
		bool ret = false;
		if(exec_mode == FileActionExecMode.DISPLAY_OUTPUT) {
			try{
				int exit_status;
				ret = Process.spawn_command_line_sync(exec_cmd, out output, 
													   null, out exit_status);
				if(ret)
					ret = (exit_status == 0);
			}
			catch(SpawnError err) {
			}
		}
		else {
			/*
			AppInfoCreateFlags flags = AppInfoCreateFlags.NONE;
			if(startup_notify)
				flags |= AppInfoCreateFlags.SUPPORTS_STARTUP_NOTIFICATION;
			if(exec_mode == FileActionExecMode.TERMINAL || 
			   exec_mode == FileActionExecMode.EMBEDDED)
				flags |= AppInfoCreateFlags.NEEDS_TERMINAL;
			GLib.AppInfo app = Fm.AppInfo.create_from_commandline(exec, null, flags);
			stdout.printf("Execute command line: %s\n\n", exec);
			ret = app.launch(null, ctx);
			*/

			// NOTE: we cannot use GAppInfo here since GAppInfo does
			// command line parsing which involving %u, %f, and other
			// code defined in desktop entry spec.
			// This may conflict with DES EMA parameters.
			// FIXME: so how to handle this cleaner?
			// Maybe we should leave all %% alone and don't translate
			// them to %. Then GAppInfo will translate them to %, not
			// codes specified in DES.
			try {
				ret = Process.spawn_command_line_async(exec_cmd);
			}
			catch(SpawnError err) {
			}
		}
		return ret;
	}

	public bool launch(AppLaunchContext ctx, List<FileInfo> files, out string? output) {
		bool plural_form = FileActionParameters.is_plural(exec);
		bool ret;
		if(plural_form) { // plural form command, handle all files at a time
			ret = launch_once(ctx, files.first().data, files, out output);
		}
		else { // singular form command, run once for each file
			StringBuilder all_output = null;
			if(output != null)
				all_output = new StringBuilder();
			foreach(unowned FileInfo fi in files) {
				string one_output;
				launch_once(ctx, fi, files, out one_output);
				if(all_output != null && one_output != null) {
					// FIXME: how to handle multiple output strings properly?
					all_output.append(one_output); // is it ok to join them all?
					all_output.append("\n");
				}
			}
			if(all_output != null && output != null)
				output = (owned) all_output.str;
			ret = true;
		}
		return ret;
	}

	public bool match(List<FileInfo> files) {
		stdout.printf("  match profile: %s\n", id);
		return condition.match(files);
	}

	public string id;
	public string? name;
	public string exec;
	public string? path;
	public FileActionExecMode exec_mode;
	public bool startup_notify;
	public string? startup_wm_class;
	public string? exec_as;

	public FileActionCondition condition;
}

}
