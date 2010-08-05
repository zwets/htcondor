/***************************************************************
 *
 * Copyright (C) 1990-2007, Condor Team, Computer Sciences Department,
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
#include "autocluster.h"
#include "condor_config.h"
#include "condor_attributes.h"


AutoCluster::AutoCluster()
{
	significant_attrs = NULL;
	old_sig_attrs = NULL; 
	sig_attrs_came_from_config_file = false;
	array.fill(NULL);
}


AutoCluster::~AutoCluster() 
{
	clearArray();
	if ( old_sig_attrs ) free(old_sig_attrs);
	if ( significant_attrs ) delete significant_attrs;
}


void AutoCluster::clearArray()
{
	int i;
	int size = array.getlast() + 1;
	for (i=0; i < size ; i++) {
		if ( array[i] ) {
			delete array[i];
			array[i] = NULL;
		}
	}
	array.truncate(-1);
}
	
bool AutoCluster::config(const char* significant_target_attrs)
{
	bool sig_attrs_changed = false;
	char *new_sig_attrs =  param ("SIGNIFICANT_ATTRIBUTES");

	dprintf(D_FULLDEBUG,
		"AutoCluster:config(%s) invoked\n",
		significant_target_attrs ? significant_target_attrs : "(null)" );

		// Handle the case where our significant attributes came from
		// the config file, and now the user removed them from the 
		// config file.  In this case, we want to wipe the slate clean.
	if ( sig_attrs_came_from_config_file && !new_sig_attrs ) {
		sig_attrs_changed = true;
		delete significant_attrs;
		significant_attrs = NULL;
		sig_attrs_came_from_config_file = false;
	}

	if ( new_sig_attrs ) {
		if ( sig_attrs_came_from_config_file == false ) {
			// in this case, we were autocomputing sig attrs, and now
			// they are set in the config file -- so wipe the slate clean.
			sig_attrs_changed = true;
			delete significant_attrs;
			significant_attrs = NULL;
		}
		sig_attrs_came_from_config_file = true;
	} 

		// Always use what the user specifies in the config file.
		// If the user did not specify anything, then we want to use
		// the significant attrs passed to us (that likely originated
		// from the matchmaker).
	if ( !new_sig_attrs && significant_target_attrs ) {
		new_sig_attrs = strdup(significant_target_attrs);
		sig_attrs_came_from_config_file = false;
	}

		// If no new_sig_attrs, and sig_attrs_changed not flagged, then
		// nothing is new and we have nothing more to do.
	if ( !new_sig_attrs && !sig_attrs_changed ) {
		return false;
	}


	if ( significant_attrs && old_sig_attrs  && (strcasecmp(new_sig_attrs,old_sig_attrs)==0) )
	{
		/* 	Just compare new and old attr strings directly.
			If they match, then we already have dealt with the
			significant attributes listed in the new string, so
			we can keep all our state (i.e. our array) and just
			return.
			We do this direct comparison because it is a common
			case that will allow us to bail out early before
			we do the more expensive work below.
		*/
		free(new_sig_attrs);	// don't leak - param() calls malloc
		return false;
	}

	if ( ! significant_attrs ) {
			// Create significant_attrs from new_sig_attrs
		significant_attrs = new StringList(new_sig_attrs);
		sig_attrs_changed = true;
	} else {
			// Merge everythying in new_sig_attrs into our existing
			// significant_attrs.  Take note if significant_attrs changed,
			// since we need to return this info to our caller.
		StringList new_attrs(new_sig_attrs);
		sig_attrs_changed = significant_attrs->create_union(new_attrs,true);
	}
	
		// the SIGNIFICANT_ATTRIBUTES setting changed, purge our
		// state.
	if ( sig_attrs_changed ) {
		clearArray();
	}

		// update old_sig_attrs
	if ( old_sig_attrs ) {
		free(old_sig_attrs);
		old_sig_attrs = NULL;

	}
	old_sig_attrs = new_sig_attrs;

	if ( sig_attrs_changed ) {
		dprintf(D_ALWAYS,
			"AutoCluster:config() significant atttributes changed to %s\n",
			new_sig_attrs ? new_sig_attrs : "(null)");
	} else {
		dprintf(D_FULLDEBUG,
			"AutoCluster:config() significant atttributes unchanged\n");
	}
		

	return sig_attrs_changed;
}

void AutoCluster::mark()
{
	int i;
	int size = array.getlast() + 1;
	for (i=0; i < size ; i++) {
		mark_array[i] = true;
	}
	mark_array.truncate(size - 1);
}

void AutoCluster::sweep()
{
	int i;
	int size = mark_array.getlast() + 1;
		// now remove any entries still marked
	for (i=0; i < size ; i++) {
		if ( mark_array[i] ) {
				// found an entry to remove.
			mark_array[i] = false;
			dprintf(D_FULLDEBUG,"removing auto cluster id %d\n",i);
			if ( array[i] ) {
				delete array[i];
				array[i] = NULL;
			}
		}
	}
}

int AutoCluster::getAutoClusterid( ClassAd *job )
{
	int cur_id = -1;
	int i;

		// first check if condor_config file even desires this
		// functionality...
	if ( !significant_attrs ) {
		return -1;
	}

    job->LookupInteger(ATTR_AUTO_CLUSTER_ID, cur_id);
	if ( cur_id != -1 ) {
			// we've previously figured it out...
		
			// tag it as touched
		mark_array[cur_id] = false;

		ASSERT( array[cur_id] != NULL );
		return cur_id;
	}

		// summarize job into a string "signature"
		// first put significant attrs from target into the signature
	MyString signature;
	char *buf;
	significant_attrs->rewind();
	const char* next_attr = NULL;
	while ( (next_attr=significant_attrs->next()) != NULL ) {
		buf = NULL;
		buf = job->sPrintExpr(NULL,0,next_attr);
		if (buf) {
			signature += buf;
			free(buf);
		}
	}
		// now put significant attrs from self into the signature.
		// note: only do this if significant_attributes is not explicitly
		// defined in our config file; if it is in our condor_config, then
		// we only want to consider the attributes listed by the admin.
	StringList internal_refs;	// this is what we want to know
	if ( !sig_attrs_came_from_config_file ) {
		// get all internal references in the job ad.
		StringList external_refs;	// we do not care about these
		job->GetReferences(ATTR_REQUIREMENTS,internal_refs,external_refs);
		internal_refs.remove_anycase(ATTR_CURRENT_TIME);	// never want this attr
		internal_refs.append(ATTR_REQUIREMENTS);	// always want these attrs
		internal_refs.append(ATTR_NICE_USER);
		internal_refs.append(ATTR_CONCURRENCY_LIMITS);

		internal_refs.rewind();
		next_attr = NULL;
		while ( (next_attr=internal_refs.next()) != NULL ) {
				// skip this attr if already in our signature from above...
			if ( significant_attrs->contains_anycase(next_attr) ) {
				internal_refs.deleteCurrent();
				continue;
			}
			buf = NULL;
			buf = job->sPrintExpr(NULL, 0,next_attr);
			if (buf) {
				signature += buf;
				free(buf);
			}
		}
	}

		// try to find a fit
	int size = array.getlast() + 1;
	for (i=0; i < size ; i++) {
		if ( array[i] == NULL ) {
			continue;
		}
		if ( signature == *(array[i]) ) {
				// found a match... update job ad
			cur_id = i;
		}
	}

	if ( cur_id == -1 ) {
			// failed to find a fit; need to create a new "cluster"
		for (i=0; array[i]; i++);	// set i to first NULL entry
		cur_id = i;
		array[cur_id] = new MyString(signature);
	}

		// put the new auto cluster id into the job ad to cache it.
	job->Assign(ATTR_AUTO_CLUSTER_ID,cur_id);

		// tag it as touched
	mark_array[cur_id] = false;

		// for some nice feedback, place the final list of attrs used to create this
		// signature into the job ad.
		// the ATTR_AUTO_CLUSTER_ATTRS attribute is also used by SetAttribute()
		// in qmgmt -- if any of the attrs used to create the signature are
		// changed, then SetAttribute() will delete the ATTR_AUTO_CLUSTER_ID, since
		// the signature needs to be recomputed as it may have changed.
	MyString final_list;
	final_list += ATTR_AUTO_CLUSTER_ATTRS;
	final_list += "=\"";
	char *tmp;
	bool need_comma = false;
	tmp = significant_attrs->print_to_string();
	if (tmp) {
		final_list += tmp;
		need_comma = true;
		free(tmp);
	}
	tmp = internal_refs.print_to_string();
	if (tmp) {
		if ( need_comma ) {
			final_list += ',';
		}
		final_list += tmp;
		free(tmp);
	}
	final_list += "\"";
	job->Insert(final_list.Value());


	return cur_id;
}
