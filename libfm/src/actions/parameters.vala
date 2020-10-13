//      parameters.vala
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
	
namespace FileActionParameters {

public string? expand(string? templ, List<FileInfo> files, bool for_display = false, FileInfo? first_file = null) {
	if(templ == null)
		return null;
	var result = new StringBuilder();
	var len = templ.length;

	if(first_file == null)
		first_file = files.first().data;

	for(var i = 0; i < len; ++i) {
		var ch = templ[i];
		if(ch == '%') {
			++i;
			ch = templ[i];
			switch(ch) {
			case 'b':	// (first) basename
				if(for_display)
					result.append(Filename.display_name(first_file.get_name()));
				else
					result.append(Shell.quote(first_file.get_name()));
				break;
			case 'B':	// space-separated list of basenames
				foreach(unowned FileInfo fi in files) {
					if(for_display)
						result.append(Shell.quote(Filename.display_name(fi.get_name())));
					else
						result.append(Shell.quote(fi.get_name()));
					result.append_c(' ');
				}
				if(result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'c':	// count of selected items
				result.append_printf("%u", files.length());
				break;
			case 'd':	// (first) base directory
				// FIXME: should the base dir be a URI?
				unowned Fm.Path base_dir = first_file.get_path().get_parent();
				var str = base_dir.to_str();
				if(for_display)
					str = Filename.display_name(str);
				result.append(Shell.quote(str));
				break;
			case 'D':	// space-separated list of base directory of each selected items
				foreach(unowned FileInfo fi in files) {
					unowned Fm.Path base_dir = fi.get_path().get_parent();
					var str = base_dir.to_str();
					if(for_display)
						str = Filename.display_name(str);
					result.append(Shell.quote(str));
					result.append_c(' ');
				}
				if(result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'f':	// (first) file name
				var filename = first_file.get_path().to_str();
				if(for_display)
					filename = Filename.display_name(filename);
				result.append(Shell.quote(filename));
				break;
			case 'F':	// space-separated list of selected file names
				foreach(unowned FileInfo fi in files) {
					var filename = fi.get_path().to_str();
					if(for_display)
						filename = Filename.display_name(filename);
					result.append(Shell.quote(filename));
					result.append_c(' ');
				}
				if(result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'h':	// hostname of the (first) URI
				// FIXME: how to support this correctly?
				// FIXME: currently we pass g_get_host_name()
				result.append(Environment.get_host_name());
				break;
			case 'm':	// mimetype of the (first) selected item
				unowned FileInfo fi = files.first().data;
				result.append(fi.get_mime_type().get_type());
				break;
			case 'M':	// space-separated list of the mimetypes of the selected items
				foreach(unowned FileInfo fi in files) {
					result.append(fi.get_mime_type().get_type());
					result.append_c(' ');
				}
				break;
			case 'n':	// username of the (first) URI
				// FIXME: how to support this correctly?
				result.append(Environment.get_user_name());
				break;
			case 'o':	// no-op operator which forces a singular form of execution when specified as first parameter,
			case 'O':	// no-op operator which forces a plural form of execution when specified as first parameter,
				break;
			case 'p':	// port number of the (first) URI
				// FIXME: how to support this correctly?
				// result.append("0");
				break;
			case 's':	// scheme of the (first) URI
				var uri = first_file.get_path().to_uri();
				result.append(Uri.parse_scheme(uri));
				break;
			case 'u':	// (first) URI
				result.append(first_file.get_path().to_uri());
				break;
			case 'U':	// space-separated list of selected URIs
				foreach(unowned FileInfo fi in files) {
					result.append(fi.get_path().to_uri());
					result.append_c(' ');
				}
				if(result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'w':	// (first) basename without the extension
				unowned string basename = first_file.get_name();
				int pos = basename.last_index_of_char('.');
				// FIXME: handle non-UTF8 filenames
				if(pos == -1)
					result.append(Shell.quote(basename));
				else {
					result.append(Shell.quote(basename[0:pos]));
				}
				break;
			case 'W':	// space-separated list of basenames without their extension
				foreach(unowned FileInfo fi in files) {
					unowned string basename = fi.get_name();
					int pos = basename.last_index_of_char('.');
					// FIXME: for_display ? Shell.quote(str) : str);
					if(pos == -1)
						result.append(Shell.quote(basename));
					else {
						result.append(Shell.quote(basename[0:pos]));
					}
					result.append_c(' ');
				}
				if(result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'x':	// (first) extension
				unowned string basename = first_file.get_name();
				int pos = basename.last_index_of_char('.');
				unowned string ext;
				if(pos >= 0)
					ext = (string)((uint8*)basename + pos + 1);
				else
					ext = "";
				result.append(Shell.quote(ext));
				break;
			case 'X':	// space-separated list of extensions
				foreach(unowned FileInfo fi in files) {
					unowned string basename = fi.get_name();
					int pos = basename.last_index_of_char('.');
					unowned string ext;
					if(pos >= 0)
						ext = (string)((uint8*)basename + pos + 1);
					else
						ext = "";
					result.append(Shell.quote(ext));
					result.append_c(' ');
				}
				if(result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case '%':	// the % character
				result.append_c('%');
				break;
			case '\0':
				break;
			}
		}
		else {
			result.append_c(ch);
		}
	}
	return (owned) result.str;
}

public bool is_plural(string? exec) {
	if(exec == null)
		return false;
	var result = new StringBuilder();
	var len = exec.length;
	// the first relevent code encountered in Exec parameter
	// determines whether the command accepts singular or plural forms
	for(var i = 0; i < len; ++i) {
		var ch = exec[i];
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

} // namespace FileActionParameter
} // namespace Fm
