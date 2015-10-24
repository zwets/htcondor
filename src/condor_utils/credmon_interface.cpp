/***************************************************************
 *
 * Copyright (C) 1990-2011, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/



#include "condor_common.h"
#include "condor_debug.h"
#include "condor_config.h"
#include "condor_uid.h"
#include <fnmatch.h>



static int _static_credmon_pid = -1;

int get_credmon_pid() {
	if(_static_credmon_pid == -1) {
		// get pid of credmon
		MyString cred_dir;
		param(cred_dir, "SEC_CREDENTIAL_DIRECTORY");
		MyString pid_path;
		pid_path.formatstr("%s/pid", cred_dir.c_str());
		FILE* credmon_pidfile = fopen(pid_path.c_str(), "r");
		int num_items = fscanf(credmon_pidfile, "%i", &_static_credmon_pid);
		fclose(credmon_pidfile);
		if (num_items != 1) {
			_static_credmon_pid = -1;
		}
		dprintf(D_ALWAYS, "CERN: get_credmon_pid %s == %i\n", pid_path.c_str(), _static_credmon_pid);
	}
	return _static_credmon_pid;
}


bool credmon_signal_and_poll(const char* user) {

	// construct filename to poll for
	char* cred_dir = param("SEC_CREDENTIAL_DIRECTORY");
	if(!cred_dir) {
		dprintf(D_ALWAYS, "ERROR: got STORE_CRED but SEC_CREDENTIAL_DIRECTORY not defined!\n");
		return false;
	}

	// get username (up to '@' if present, else whole thing)
	char username[256];
	const char *at = strchr(user, '@');
	if(at) {
		strncpy(username, user, (at-user));
		username[at-user] = 0;
	} else {
		strncpy(username, user, 255);
		username[255] = 0;
	}

	// check to see if .cc already exists
	char ccfilename[PATH_MAX];
	sprintf(ccfilename, "%s%c%s.cc", cred_dir, DIR_DELIM_CHAR, username);

	// now signal the credmon
	pid_t credmon_pid = get_credmon_pid();
	if (credmon_pid == -1) {
		dprintf(D_ALWAYS, "ZKM: failed to get pid of credmon.\n");
		return false;
	}

	dprintf(D_ALWAYS, "ZKM: sending SIGHUP to credmon pid %i\n", credmon_pid);
	int rc = kill(credmon_pid, SIGHUP);
	if (rc == -1) {
		dprintf(D_ALWAYS, "ZKM: failed to signal credmon: %i\n", errno);
		return false;
	}

	// now poll for existence of .cc file
	int retries = 20;
	struct stat junk_buf;
	while (retries > 0) {
		rc = stat(ccfilename, &junk_buf);
		if (rc==-1) {
			dprintf(D_ALWAYS, "ZKM: errno %i, waiting for %s to appear (%i seconds left)\n", errno, ccfilename, retries);
			sleep(1);
			retries--;
		} else {
			break;
		}
	}
	if (retries == 0) {
		dprintf(D_ALWAYS, "ZKM: FAILURE: credmon never created %s after 20 seconds!\n", ccfilename);
		return false;
	}

	dprintf(D_ALWAYS, "ZKM: SUCCESS: file %s found after %i seconds\n", ccfilename, 20-retries);
	return true;
}

bool credmon_mark_creds_for_sweeping(const char* user) {

	// construct filename to create
	char* cred_dir = param("SEC_CREDENTIAL_DIRECTORY");
	if(!cred_dir) {
		dprintf(D_ALWAYS, "ERROR: got mark_creds_for_sweeping but SEC_CREDENTIAL_DIRECTORY not defined!\n");
		return false;
	}

	// get username (up to '@' if present, else whole thing)
	char username[256];
	const char *at = strchr(user, '@');
	if(at) {
		strncpy(username, user, (at-user));
		username[at-user] = 0;
	} else {
		strncpy(username, user, 255);
		username[255] = 0;
	}

	// check to see if .cc already exists
	char markfilename[PATH_MAX];
	sprintf(markfilename, "%s%c%s.mark", cred_dir, DIR_DELIM_CHAR, username);

	priv_state priv = set_root_priv();
	FILE* f = safe_fcreate_replace_if_exists(markfilename, "w", 0600);
	set_priv(priv);
	if (f == NULL) {
		dprintf(D_ALWAYS, "ERROR: safe_fcreate_replace_if_exists(%s) failed!\n", markfilename);
		return false;
	}

	fclose(f);
	return true;
}

int markfilter(const dirent*d) {
  bool match = !fnmatch("*.mark", d->d_name, FNM_PATHNAME);
  // printf("d: %s, %i\n", d->d_name, match);
  return match;
}

void process_cred_file(const char *src) {
   //char * src = fname;
   char * trg = strdup(src);
   strcpy((trg + strlen(src) - 5), ".cred");
   dprintf(D_ALWAYS, "%li: FOUND %s UNLINK %s\n", time(0), src, trg);
   unlink(trg);
   strcpy((trg + strlen(src) - 5), ".cc");
   dprintf(D_ALWAYS, "%li: FOUND %s UNLINK %s\n", time(0), src, trg);
   unlink(trg);
   strcpy((trg + strlen(src) - 5), ".mark");
   dprintf(D_ALWAYS, "%li: FOUND %s UNLINK %s\n", time(0), src, trg);
   unlink(trg);

   free(trg);
}

void credmon_sweep_creds() {
	struct dirent **namelist;
	int n;

	// construct filename to poll for
	char* cred_dir = param("SEC_CREDENTIAL_DIRECTORY");
	if(!cred_dir) {
		dprintf(D_ALWAYS, "ZKM: skipping sweep, SEC_CREDENTIAL_DIRECTORY not defined!\n");
		return;
	}

	MyString fullpathname;
	dprintf(D_ALWAYS, "ZKM: scandir(%s)\n", cred_dir);
	n = scandir(cred_dir, &namelist, &markfilter, alphasort);
	if (n >= 0) {
		while (n--) {
			fullpathname.formatstr("%s%c%s", cred_dir, DIR_DELIM_CHAR, namelist[n]->d_name);
			priv_state priv = set_root_priv();
			process_cred_file(fullpathname.c_str());
			set_priv(priv);
			free(namelist[n]);
		}
		free(namelist);
	} else {
		dprintf(D_ALWAYS, "ZKM: skipping sweep, scandir(%s) not defined!\n", cred_dir);
	}
	free(cred_dir);
}


bool credmon_clear_mark(const char* user) {
	char* cred_dir = param("SEC_CREDENTIAL_DIRECTORY");
	if(!cred_dir) {
		dprintf(D_ALWAYS, "ERROR: got STORE_CRED but SEC_CREDENTIAL_DIRECTORY not defined!\n");
		return false;
	}

	// get username (up to '@' if present, else whole thing)
	char username[256];
	const char *at = strchr(user, '@');
	if(at) {
		strncpy(username, user, (at-user));
		username[at-user] = 0;
	} else {
		strncpy(username, user, 255);
		username[255] = 0;
	}

	// unlink the "mark" file on every update
	char markfilename[PATH_MAX];
	sprintf(markfilename, "%s%c%s.mark", cred_dir, DIR_DELIM_CHAR, username);

	priv_state priv = set_root_priv();
	int rc = unlink(markfilename);
	set_priv(priv);

	if(rc) {
		// ENOENT is common and not worth reporting
		if(errno != ENOENT) {
			dprintf(D_ALWAYS, "CERN: WARNING! unlink(%s) got error %i (%s)\n",
				markfilename, errno, strerror(errno));
		}
	} else {
		dprintf(D_ALWAYS, "ZKM: cleared mark file %s\n", markfilename);
	}

	return true;
}

