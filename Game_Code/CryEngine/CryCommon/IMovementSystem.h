//////////////////////////////////////////////////////////////////////////// 
// 
//  CryEngine Source File. 
//  Copyright (C), Crytek, 1999-2012. 
// ------------------------------------------------------------------------- 
//  File name:   IMovementSystem.h
//  Version:     v1.00 
//  Created:     2011-12-20 by Jonas Johansson. 
//  Description: Interface for the AI Movement System.
// ------------------------------------------------------------------------- 
//  History: 
// 
//////////////////////////////////////////////////////////////////////////// 

#pragma once

#ifndef IMovementSystem_h
#define IMovementSystem_h

struct MovementRequest;
struct MovementRequestID;
struct MovementRequestStatus;

struct IMovementSystem
{
	virtual ~IMovementSystem() {}

	//
	// Ask the movement system to satisfy your request.
	//
	// The movement system will contact you via the callback to inform
	// you whether the request has been satisfied or not.
	//
	// If you submit a callback you are responsible to to call CancelRequest
	// before the callback becomes invalid.
	//

	virtual MovementRequestID QueueRequest(const MovementRequest& request) = 0;

	//
	// Tell the movement system you are no longer interested in getting
	// your request satisfied. Note that this doesn't necessarily mean
	// that the actor will stop, it simply means that you no longer care
	// about the outcome.
	//

	virtual void CancelRequest(const MovementRequestID& id) = 0;

	//
	// Get information about the current status of a request.
	// You'll see if it's in queue, path finding, or
	// what block of a plan it's currently executing.
	//

	virtual void GetRequestStatus(const MovementRequestID& id, MovementRequestStatus& status) const = 0;

	//
	// This resets the movement system to it's initial state.
	//
	// - The internal data will be cleaned out and reset.
	// - No callbacks will be called during reset process.
	//

	virtual void Reset() = 0;

	//
	// This is where the movement system will do it's work.
	//

	virtual void Update(float updateTime) = 0;
};

#endif // IMovementSystem_h