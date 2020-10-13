//      utils.vala
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

namespace Utils {

private string key_file_get_string(KeyFile kf, string group, string key, string? def_val=null) {
	string? val;
	try {
		val = kf.get_string(group, key);
	}
	catch(KeyFileError err) {
		val = def_val;
	}
	return val;
}

private string[]? key_file_get_string_list(KeyFile kf, string group, string key, string[]? def_val=null) {
	string[]? val;
	try {
		val = kf.get_string_list(group, key);
	}
	catch(KeyFileError err) {
		val = def_val;
	}
	return val;
}

private string? key_file_get_locale_string(KeyFile kf, string group, string key, string? def_val=null) {
	string? val;
	try {
		val = kf.get_locale_string(group, key);
	}
	catch(KeyFileError err) {
		val = def_val;
	}
	return val;
}

private bool key_file_get_bool(KeyFile kf, string group, string key, bool def_val=false) {
	bool val;
	try {
		val = kf.get_boolean(group, key);
	}
	catch(KeyFileError err) {
		val = def_val;
	}
	return val;
}

#if 0
private int key_file_get_int(KeyFile kf, string group, string key, int def_val=0) {
	int val;
	try {
		val = kf.get_integer(group, key);
	}
	catch(KeyFileError err) {
		val = def_val;
	}
	return val;
}
#endif

}
