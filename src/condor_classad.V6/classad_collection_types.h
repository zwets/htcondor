/***************************Copyright-DO-NOT-REMOVE-THIS-LINE**
 * CONDOR Copyright Notice
 *
 * See LICENSE.TXT for additional notices and disclaimers.
 *
 * Copyright (c)1990-1998 CONDOR Team, Computer Sciences Department, 
 * University of Wisconsin-Madison, Madison, WI.  All Rights Reserved.  
 * No use of the CONDOR Software Program Source Code is authorized 
 * without the express consent of the CONDOR Team.  For more information 
 * contact: CONDOR Team, Attention: Professor Miron Livny, 
 * 7367 Computer Sciences, 1210 W. Dayton St., Madison, WI 53706-1685, 
 * (608) 262-0856 or miron@cs.wisc.edu.
 *
 * U.S. Government Rights Restrictions: Use, duplication, or disclosure 
 * by the U.S. Government is subject to restrictions as set forth in 
 * subparagraph (c)(1)(ii) of The Rights in Technical Data and Computer 
 * Software clause at DFARS 252.227-7013 or subparagraphs (c)(1) and 
 * (2) of Commercial Computer Software-Restricted Rights at 48 CFR 
 * 52.227-19, as applicable, CONDOR Team, Attention: Professor Miron 
 * Livny, 7367 Computer Sciences, 1210 W. Dayton St., Madison, 
 * WI 53706-1685, (608) 262-0856 or miron@cs.wisc.edu.
 ****************************Copyright-DO-NOT-REMOVE-THIS-LINE**/


#ifndef _Collections_H
#define _Collections_H

//-------------------------------------------------------------------------

#include "matchClassad.h"
#include "Set.h"
#include "MyString.h"

int partitionHashFcn( const MyString &, int );

//-------------------------------------------------------------------------

class RankedClassAd {
public:
	RankedClassAd() { Rank=0.0; }
	RankedClassAd(const MyString& oid) { OID=oid; Rank=0.0; }
	RankedClassAd(const MyString& oid, float rank) { OID=oid; Rank=rank; }

	MyString OID;
	float Rank;

	friend bool operator==(const RankedClassAd& RankedAd1, 
		const RankedClassAd& RankedAd2) 
	{
		return ((RankedAd1.OID==RankedAd2.OID) ? true : false);
	}

	friend bool operator!=(const RankedClassAd& RankedAd1, 
		const RankedClassAd& RankedAd2) 
	{
		return ((RankedAd1.OID==RankedAd2.OID) ? false : true);
	}
};

//-------------------------------------------------------------------------
// Collection Hierarchy
//-------------------------------------------------------------------------

typedef Set<MyString> StringSet;
typedef Set<int> IntegerSet;
typedef Set<RankedClassAd> RankedAdSet;

enum CollectionType { 
	ExplicitCollection_e, 
	ConstraintCollection_e, 
	PartitionParent_e, 
	PartitionChild_e 
};

//-------------------------------------------------------------------------
class CollChildIterator;
class CollContentIterator;
class ClassAdCollection;

class BaseCollection {
public:

	BaseCollection(BaseCollection* parent, const MyString& rank);
	BaseCollection(BaseCollection* parent, ExprTree *tree);
	virtual ~BaseCollection( );

	virtual bool CheckClassAd(ClassAd* Ad)=0;
	virtual int Type()=0;

	ExprTree *GetRankExpr();
	double GetRankValue( ClassAd* ad );

	BaseCollection* GetParent() { return Parent; }

		// iterator management
	void RegisterChildItor( CollChildIterator*, ClassAdCollection*, int  );
	void UnregisterChildItor( CollChildIterator* );
	void RegisterContentItor( CollContentIterator*, ClassAdCollection*, int );
	void UnregisterContentItor( CollContentIterator* );

		// tables of active iterators
	int lastChildItor, lastContentItor;
	ExtArray<CollChildIterator*> 	childItors;
	ExtArray<CollContentIterator*> 	contentItors;

		// itor notification triggers
	void NotifyContentItorsInsertion( );
	void NotifyContentItorsDeletion( const RankedClassAd & );
	void NotifyChildItorsInsertion( );
	void NotifyChildItorsDeletion( int );

	int MemberCount() { return Members.Count(); }

	IntegerSet 		Children;
	RankedAdSet 	Members;
	MatchClassAd 	rankCtx;

protected:
    BaseCollection* Parent;
};

//-------------------------------------------------------------------------

class ExplicitCollection : public BaseCollection {
public:

	ExplicitCollection(BaseCollection* parent, const MyString& rank, bool fullFlag) 
		: BaseCollection(parent, rank) 
	{
		FullFlag=fullFlag;
	}

	ExplicitCollection(BaseCollection* parent, ExprTree *rank, bool fullFlag) 
		: BaseCollection(parent,rank) 
	{
		FullFlag=fullFlag;
	}

	virtual bool CheckClassAd(ClassAd*) { return FullFlag; }
	virtual int Type() { return ExplicitCollection_e; }

	bool FullFlag;
};

//-------------------------------------------------------------------------

class ConstraintCollection : public BaseCollection {
public:
	ConstraintCollection(BaseCollection* parent, const MyString& rank, const MyString& constraint);
	ConstraintCollection(BaseCollection* parent, ExprTree *rank, ExprTree *constraint);
	virtual bool CheckClassAd(ClassAd* Ad);

	virtual int Type() { return ConstraintCollection_e; }

	ExprTree 	*constraint;
	MatchClassAd constraintCtx;
};

//-------------------------------------------------------------------------

class PartitionParent : public BaseCollection {
public:

	PartitionParent(BaseCollection* parent, const MyString& rank, StringSet& attributes)
		: BaseCollection(parent,rank), childPartitions( 27, partitionHashFcn )
	{
		Attributes=attributes;
	}

	PartitionParent(BaseCollection* parent, ExprTree *rank, StringSet& attributes)
		: BaseCollection(parent,rank), childPartitions( 27, partitionHashFcn )
	{
		Attributes=attributes;
	}

	virtual bool CheckClassAd(ClassAd*) { return false; }
	virtual int Type() { return PartitionParent_e; }

	StringSet Attributes;
	HashTable<MyString,int> childPartitions;
};

//-------------------------------------------------------------------------

class PartitionChild : public BaseCollection {
public:

	PartitionChild(BaseCollection* parent, ExprTree *rank, MyString& values) :BaseCollection(parent,rank) 
	{
		PartitionValues=values;
	}

	virtual bool CheckClassAd(ClassAd*);
	virtual int Type() { return PartitionChild_e; }

	MyString PartitionValues;
};

//-------------------------------------------------------------------------

/// The state of an iterator
enum CollItorStatus {
	/** Nil State */	
		COLL_NIL_STATUS		= 0,
	/** Iterator OK (i.e., can access elements, can be moved) */ 
		COLL_ITOR_OK 		= ( 1 << 0 ),
	/** Iterator before first item (internal state) */ 
		COLL_BEFORE_START	= ( 1 << 1 ),
	/** Iterator at end of item list */ 
		COLL_AT_END	 		= ( 1 << 2 ),
	/** Iterator was moved automagically */ 
		COLL_ITOR_MOVED		= ( 1 << 3 ),
	/** Items were added to set being iterated over */ 
		COLL_ITEM_ADDED 	= ( 1 << 4 ),
	/** Items were removed from set being iterated over */ 
		COLL_ITEM_REMOVED 	= ( 1 << 5 ),
	/** Iterator is invalid (i.e., cannot be used to access elements or be 
	 	moved ) */ 
		COLL_ITOR_INVALID	= ( 1 << 6 )
};

class ClassAdCollection;

/** Class for iterating through the child collections of a collection. */
class CollChildIterator {
friend class BaseCollection;
public:
	/// Constructor
	CollChildIterator( );
	/** Copy constructor.  The new object is a distinct and independent 
	 	iterator whose initial position and/or state is the same as the
		parameter.
		@param i Another child collection iterator
	*/
	CollChildIterator( const CollChildIterator & );
	/** Destructor.  If the iterator is active, the destructor unregisters
	 	itself from the collection it is iterating over
	*/
	~CollChildIterator( );

	/**@name Iteration Control Methods */
	//@{
	/** Get the ID of the child collection the iterator currently points to.
	 	(The state of the iterator will be set to OK if the operation 
		succeeded.)
	 	@param ID The ID of the child collection.  If the function returns false
			the value of this parameter is undefined.
		@return false if the iterator does not currently point to a valid
			child collection, true otherwise
	*/
	bool CurrentCollection( int &ID );

	/** Move the iterator and get the ID of the next child collection.
	 	(The previous state of the iterator will be cleared.)
	 	@param ID The ID of the next child collection
	 	@return 0 if the iterator is invalid, or has gone past the last child 
			collection, non-zero if the iterator was successfully moved.
			Additionally, the return value is {\bf negative} if child 
			collections were added or removed to the parent collection since
			the last iteration operation to this iterator.  This situation may
			have already caused the iterator to move.
	*/
	int  NextCollection( int &ID );

	/** Move the iterator to the next child collection.
	 	(The previous state of the iterator will be cleared.)
	 	@return 0 if the iterator is invalid, or has gone past the last child 
			collection, non-zero if the iterator was successfully moved.
			Additionally, the return value is {\bf negative} if child 
			collections were added or removed to the parent collection since
			the last iteration operation to this iterator.  This situation may
			have already caused the iterator to move.
	*/
	int  NextCollection( ) { int ID; return( NextCollection( ID ) ); }
	//@}

	/**@name State Interrogation Methods */
	//@{
	/** Get the iterator status.
	 	@return The status of the iterator
		@see CollIteratorStatus
	*/
	int  GetIteratorStatus( ) const { return( status ); }

	/** Check if the the iterator is valid and pointing at an element
	 	@return true if the iterator is currently pointing at an element, 
			false otherwise
	*/
	bool IteratorOk( ) const { return( status & COLL_ITOR_OK ); }

	/** Check if the iterator has passed over the end of the list.
	 	@return true if the iterator is at the end of the list, and false
			otherwise
	*/
	bool AtEnd( ) const { return( status & COLL_AT_END ); }

	/** Check if the iterator was moved independent of explicit move calls.
	 	This may occur if the element which was pointed at was deleted, forcing
		the iterator to move over to the next element.
		@return true if the above situation occurred, false otherwise.
	*/
	bool IteratorMoved( ) const { return( status & COLL_ITOR_MOVED ); }

	/** Check if child collections were created after the iteration was begun.
	 	The new children may not be visited by the iterator.
		@return true if the above situation occured, false otherwise.
	*/
	bool CollectionsAdded( ) const { return( status & COLL_ITEM_ADDED ); }

	/** Check if child collections were removed after the iteration was begun.
	 	If the deleted child collection was under the iterator at delete time, 
		the iterator would have been moved, and the COLL_ITER_MOVED would be 
		set.
		@return true if the above situation occured, false otherwise.
	*/
	bool CollectionsRemoved( ) const { return( status & COLL_ITEM_REMOVED ); }

	/** Check if the iterator is invalid.  If the iterator was not initialized,
	 	or if the collection being iterated over is deleted, the iterator is
		moved to the invalid state.
		@return true if the iterator is invalid and false otherwise.
	*/
	bool IsInvalid( ) const { return( status & COLL_ITOR_INVALID ); }
	//@}

private:
	void initialize(ClassAdCollection*,BaseCollection*,int,const IntegerSet&);
	void updateForInsertion( );
	void updateForDeletion( int );
	void invalidate( );

	int	 				collID;
	BaseCollection		*baseCollection;
	ClassAdCollection	*collManager;
	SetIterator<int> 	itor;
	int					status;
};


/** Class to iterate over the content of a collection. (i.e., iterate over the
 	classads in the collection)
*/
class CollContentIterator {
friend class BaseCollection;
public:
	/// Constructor
	CollContentIterator( );
    /** Copy constructor.  The new object is a distinct and independent 
	    iterator whose initial position and/or state is the same as the
	    parameter.
	    @param i Another content collection iterator
	*/
	CollContentIterator( const CollContentIterator & );
	/** Destructor. If the iterator is active, the destructor unregisters 
	 	itself from the collection it is iterating over.
	*/
	~CollContentIterator( );
	
	/**@name Iteration Control Methods */
	//@{
	/** Get the classad currenly under the iterator. (The state of the iterator 
	 	will be reset to OK.)
	 	@param ad A reference to a ClassAd pointer, which points to an undefined
			location if the function returns false.
		@return true if the iterator is currently over a classad, false 
			otherwise
	*/
	bool CurrentAd( const ClassAd *&ad );
	
	/** Get the key of the current classad.  (The state of the iterator will
	 	be reset to OK.)
	 	@param key A character buffer into which the key of the current element
			will be copied.  The contents of this buffer is undefined if the
			iterator is not currently pointing to an element (i.e., returns 
			false).
		@return true if the iterator is currently pointing to a valid element,
			false otherwise.
	*/
	bool CurrentAdKey( char *key );
	
	/** Get the rank of the current classad.  (The state of the iterator will
	 	be reset to OK.)
	 	@param rank The rank of the classad.  This parameter will have an
			undefined value if the function returns false.
		@return true if the iterator is currently pointing to a valid element,
			false otherwise.
	*/
	bool CurrentAdRank( double &rank );
	
	/** Get the next classad in the collection.  (The previous state of the
	 	iterator will be cleared.)
	 	@param ad A reference to a ClasssAd pointer, which points to the next
			classad in the collection if the iterator was moved successfully to
			the next element, or undefined otherwise.
		@return 0 if the iterator was invalid or could not be moved 
			successfully, and non-zero otherwise.
			Additionally, the return value is {\bf negative} if classads were 
			were added or removed to the collection being iterated over since
			the last iteration operation to the iterator.  This situation may
			have already caused the iterator to move.
	*/
	int  NextAd( const ClassAd *&ad );
	
	/** Move iterator to the next classad in the collection.  (The previous
	 	state of the iterator will be cleared.)
		@return 0 if the iterator was invalid or could not be moved 
			successfully, and non-zero otherwise.
			Additionally, the return value is {\bf negative} if classads were 
			were added or removed to the collection being iterated over since
			the last iteration operation to the iterator.  This situation may
			have already caused the iterator to move.
	*/
	int  NextAd( ) {  ClassAd *ad; return( NextAd( ad ) ); }
	//@}

	/**@name State Interrogation Methods */
	//@{
	/** Get the status of the iterator.
	 	@return The iterator status
		@see CollIteratorStatus
	*/
	int  GetIteratorStatus( ) const { return( status ); }

    /** Check if the the iterator is valid and pointing at an element
        @return true if the iterator is currently pointing at an element, 
        	false otherwise
    */
	bool IteratorOk( ) const { return( status & COLL_ITOR_OK ); }
	
	/** Check if the iterator has passed over the end of the collection.
	 	@return true if the above situation occurred, false otherwise.
	*/
	bool AtEnd( ) const { return( status & COLL_AT_END ); }
	
    /** Check if the iterator was moved independent of explicit move calls.
     	This may occur if the element which was pointed at was deleted, forcing
     	the iterator to move over to the next element.
     @return true if the above situation occurred, false otherwise.
     */
	bool IteratorMoved( ) const { return( status & COLL_ITOR_MOVED ); }
	
    /** Check if new classads were added after the iteration was begun.
     		The new children may not be visited by the iterator.
     	@return true if the above situation occured, false otherwise.
     */
	bool ClassAdsAdded( ) const { return( status & COLL_ITEM_ADDED ); }

    /** Check if classads were removed after the iteration was begun.
     	If the deleted classad was under the iterator at delete time, the 
		iterator would have been moved, and the COLL_ITER_MOVED would be set.
     @return true if the above situation occured, false otherwise.
     */

	bool ClassAdsRemoved( ) const { return( status & COLL_ITEM_REMOVED ); }

    /** Check if the iterator is invalid.  If the iterator was not initialized,
     	or if the collection being iterated over is deleted, the iterator is
     	moved to the invalid state.
     @return true if the iterator is invalid and false otherwise.
     */
	bool IsInvalid( ) const { return( status & COLL_ITOR_INVALID ); }
	//@}
	
private:
	void initialize(ClassAdCollection*,BaseCollection*,int,const RankedAdSet&);
	void updateForInsertion( );
	void updateForDeletion( const RankedClassAd & );
	void invalidate( );

	int							collID;
	BaseCollection 				*baseCollection;
	ClassAdCollection			*collManager;
	SetIterator<RankedClassAd> 	itor;
	int							status;
};

#endif
