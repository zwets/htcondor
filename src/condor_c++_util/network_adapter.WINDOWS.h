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

#ifndef _NETWORK_ADAPTER_WINDOWS_H_
#define _NETWORK_ADAPTER_WINDOWS_H_

/***************************************************************
 * Headers
 ***************************************************************/

#include "network_adapter.h"
#include "condor_constants.h"
#include "setup_api_dll.h"
#include <iptypes.h>

/***************************************************************
 * WindowsNetworkAdapter class
 ***************************************************************/

class WindowsNetworkAdapter : NetworkAdapterBase
{

public:

	/** @name Instantiation.
	*/
	//@{

    /// Constructor
	WindowsNetworkAdapter () throw ();

	/// Constructor
	WindowsNetworkAdapter ( LPCSTR ip_addr, unsigned int ) throw ();

    /// Alternate
    WindowsNetworkAdapter ( LPCSTR description ) throw ();

	/// Constructor -- not implemented
	WindowsNetworkAdapter ( const char * /*name*/ ) throw () { };

	/// Destructor
	virtual ~WindowsNetworkAdapter () throw (); 

	//@}

	/** @name Device properties.
	*/
	//@{

	/** Returns the adapter's hardware address
		@return a string representation of the addapter's hardware
        address
	*/
	const char* hardwareAddress (void) const;

    /** Returns the adapter's IP address as a string
		@return the adapter's's IP address
	*/
	virtual unsigned ipAddress (void) const;

    /** Returns the adapter's hardware address
		@return a string representation of the subnet mask
	*/
	const char* subnet (void) const;

    /** Returns the adapter's hardware address
		@return a string representation of the addapter's hardware
        address
	*/
	bool wakeAble (void) const;

    /** Checks that the adapter actually exists
        @returns true if the adapter exists on the machine; 
        otherwise, false.
	    */
	bool exists () const;

    /** Initialize the internal structures (can be called multiple
        times--such as in the case of a reconfiguration) 
		@return true if it was succesful; otherwise, false.
		*/
    bool initialize ();

	//@}

    /** @name Device parameters.
	Basic Plug and Play device properties.
	*/
	//@{

	/** Returns the device's power information
		@return The function retrieves the device's power management
		information. Use LocalFree) to release the memory.
		*/
	PCM_POWER_DATA getPowerData () const;

   //@}

private:

    /** Data members */
    CHAR _ip_address[IP_STRING_BUF_SIZE],
         _description[MAX_ADAPTER_DESCRIPTION_LENGTH + 4],
         _hardware_address[32],
         _subnet[IP_STRING_BUF_SIZE],
         _adapter_name[MAX_ADAPTER_NAME_LENGTH + 4];
    bool _wake_able,
         _exists;    

    /**	Some registry values require some preprocessing before they can
		be queried, so we allow a user to specify a function to handle
		preprocessing.
	*/
	typedef void (*PRE_PROCESS_REISTRY_VALUE)(PBYTE);

	/** Returns the device's requested property
		@return The function retrieves a buffer to the device's 
		requested  information. Use LocalFree) to release the memory.
		@param ID of the property to query.
		@param Preprocessing function.
		@see Registry Keys for Drivers in the MS DDK.
		*/
	PBYTE getRegistryProperty (
		IN DWORD registry_property,
		IN PRE_PROCESS_REISTRY_VALUE preprocess = NULL ) const;
	
	
};

#define NETWORK_ADAPTER_TYPE_DEFINED	1
typedef WindowsNetworkAdapter NetworkAdapter;


#endif //  _NETWORK_ADAPTER_WINDOWS_H_
