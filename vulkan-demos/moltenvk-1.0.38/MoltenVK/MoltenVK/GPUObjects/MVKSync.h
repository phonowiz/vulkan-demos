/*
 * MVKSync.h
 *
 * Copyright (c) 2014-2019 The Brenwill Workshop Ltd. (http://www.brenwill.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "MVKDevice.h"
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_set>

class MVKFenceSitter;


#pragma mark -
#pragma mark MVKSemaphoreImpl

/** 
 * A general utility semaphore object. Reservations can be made with an instance, 
 * and it will block waiting threads until reservations have been released.
 *
 * An instance can be configured so that each call to the reserve() function must be
 * matched with a separate call to the release() function before waiting threads are
 * unblocked, or it can be configured so that a single call to the release() function
 * will release all outstanding reservations and unblock all threads immediately.
 */
class MVKSemaphoreImpl : public MVKBaseObject {

public:

	/** Returns nil as this object has no need to track the Vulkan object. */
	MVKVulkanAPIObject* getVulkanAPIObject() override { return nullptr; };

	/**
	 * Adds a reservation to this semaphore, incrementing the reservation count.
	 * Subsequent calls to a wait() function will block until a corresponding call
     * is made to the release() function.
	 */
	void reserve();

	/**
	 * Depending on configuration, releases one or all reservations. When all reservations
	 * have been released, unblocks all waiting threads to continue processing.
	 * Returns true if the last reservation was released.
	 */
	bool release();

	/** Returns whether this instance is in a reserved state. */
	bool isReserved();

	/**
	 * Blocks processing on the current thread until any or all (depending on configuration) outstanding
     * reservations have been released, or until the specified timeout interval in nanoseconds expires.
	 *
	 * If timeout is the special value UINT64_MAX the timeout is treated as infinite.
     *
     * If reserveAgain is set to true, a single reservation will be added once this wait is finished.
	 *
	 * Returns true if all reservations were cleared, or false if the timeout interval expired.
	 */
	bool wait(uint64_t timeout = UINT64_MAX, bool reserveAgain = false);


#pragma mark Construction

	/** 
	 * Constructs an instance with the specified number of initial reservations. 
	 * This value defaults to zero, starting the semaphore in an unblocking state.
	 *
	 * The waitAll parameter indicates whether a call to the release() function is required
	 * for each call to the reserve() function (waitAll = true), or whether a single call 
	 * to the release() function will release all outstanding reservations (waitAll = false). 
	 * This value defaults to true, indicating that each call to the reserve() function will
	 * require a separate call to the release() function to cause the semaphore to stop blocking.
	 */
    MVKSemaphoreImpl(bool waitAll = true, uint32_t reservationCount = 0)
        : _shouldWaitAll(waitAll), _reservationCount(reservationCount) {}

    /** Destructor. */
    ~MVKSemaphoreImpl();


private:
	bool operator()();
    inline bool isClear() { return _reservationCount == 0; }    // Not thread-safe

	std::mutex _lock;
	std::condition_variable _blocker;
	uint32_t _reservationCount;
	bool _shouldWaitAll;
};


#pragma mark -
#pragma mark MVKSemaphore

/** Abstract class that represents a Vulkan semaphore. */
class MVKSemaphore : public MVKVulkanAPIDeviceObject {

public:

	/** Returns the Vulkan type of this object. */
	VkObjectType getVkObjectType() override { return VK_OBJECT_TYPE_SEMAPHORE; }

	/** Returns the debug report object type of this object. */
	VkDebugReportObjectTypeEXT getVkDebugReportObjectType() override { return VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT; }

	/**
	 * Wait for this semaphore to be signaled.
	 *
	 * If the subclass uses command encoding AND the mtlCmdBuff is not nil, a wait
	 * is encoded on the mtlCmdBuff, and this call returns immediately. Otherwise, if the
	 * subclass does NOT use command encoding, AND the mtlCmdBuff is nil, this call blocks
	 * indefinitely until this semaphore is signaled. Other combinations do nothing.
	 *
	 * This design allows this function to be blindly called twice, from different points in the
	 * code path, once with a mtlCmdBuff to support encoding a wait on the command buffer if the
	 * subclass supports command encoding, and once without a mtlCmdBuff, at the point in the
	 * code path where the code should block if the subclass does not support command encoding.
	 */
	virtual void encodeWait(id<MTLCommandBuffer> mtlCmdBuff) = 0;

	/**
	 * Signals this semaphore.
	 *
	 * If the subclass uses command encoding AND the mtlCmdBuff is not nil, a signal is
	 * encoded on the mtlCmdBuff. Otherwise, if the subclass does NOT use command encoding,
	 * AND the mtlCmdBuff is nil, this call immediately signals any waiting calls.
	 * Either way, this call returns immediately. Other combinations do nothing.
	 *
	 * This design allows this function to be blindly called twice, from different points in the
	 * code path, once with a mtlCmdBuff to support encoding a wait on the command buffer if the
	 * subclass supports command encoding, and once without a mtlCmdBuff, at the point in the
	 * code path where the code should block if the subclass does not support command encoding.
	 */
	virtual void encodeSignal(id<MTLCommandBuffer> mtlCmdBuff) = 0;

	/** Returns whether this semaphore uses command encoding. */
	virtual bool isUsingCommandEncoding() = 0;


#pragma mark Construction

    MVKSemaphore(MVKDevice* device, const VkSemaphoreCreateInfo* pCreateInfo) : MVKVulkanAPIDeviceObject(device) {}

protected:
	void propogateDebugName() override {}

};


#pragma mark -
#pragma mark MVKSemaphoreMTLFence

/** An MVKSemaphore that uses MTLFence to provide synchronization. */
class MVKSemaphoreMTLFence : public MVKSemaphore {

public:
	void encodeWait(id<MTLCommandBuffer> mtlCmdBuff) override;
	void encodeSignal(id<MTLCommandBuffer> mtlCmdBuff) override;
	bool isUsingCommandEncoding() override { return true; }

	MVKSemaphoreMTLFence(MVKDevice* device, const VkSemaphoreCreateInfo* pCreateInfo);

	~MVKSemaphoreMTLFence() override;

protected:
	id<MTLFence> _mtlFence;
};


#pragma mark -
#pragma mark MVKSemaphoreMTLEvent

/** An MVKSemaphore that uses MTLEvent to provide synchronization. */
class MVKSemaphoreMTLEvent : public MVKSemaphore {

public:
	void encodeWait(id<MTLCommandBuffer> mtlCmdBuff) override;
	void encodeSignal(id<MTLCommandBuffer> mtlCmdBuff) override;
	bool isUsingCommandEncoding() override { return true; }

	MVKSemaphoreMTLEvent(MVKDevice* device, const VkSemaphoreCreateInfo* pCreateInfo);

	~MVKSemaphoreMTLEvent() override;

protected:
	id<MTLEvent> _mtlEvent;
	std::atomic<uint64_t> _mtlEventValue;
};


#pragma mark -
#pragma mark MVKSemaphoreEmulated

/** An MVKSemaphore that uses CPU synchronization to provide synchronization functionality. */
class MVKSemaphoreEmulated : public MVKSemaphore {

public:
	void encodeWait(id<MTLCommandBuffer> mtlCmdBuff) override;
	void encodeSignal(id<MTLCommandBuffer> mtlCmdBuff) override;
	bool isUsingCommandEncoding() override { return false; }

	MVKSemaphoreEmulated(MVKDevice* device, const VkSemaphoreCreateInfo* pCreateInfo);

protected:
	MVKSemaphoreImpl _blocker;
};


#pragma mark -
#pragma mark MVKFence

/** Represents a Vulkan fence. */
class MVKFence : public MVKVulkanAPIDeviceObject {

public:

	/** Returns the Vulkan type of this object. */
	VkObjectType getVkObjectType() override { return VK_OBJECT_TYPE_FENCE; }

	/** Returns the debug report object type of this object. */
	VkDebugReportObjectTypeEXT getVkDebugReportObjectType() override { return VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT; }

	/**
	 * If this fence has not been signaled yet, adds the specified fence sitter to the
	 * internal list of fence sitters that will be notified when this fence is signaled,
	 * and then calls addUnsignaledFence() on the fence sitter so it is aware that it
	 * will be signaled.
	 *
	 * Does nothing if this fence has already been signaled, and does not call 
	 * addUnsignaledFence() on the fence sitter.
	 *
	 * Each fence sitter should only listen once for each fence. Adding the same fence sitter
	 * more than once in between each fence reset and signal results in undefined behaviour.
	 */
	void addSitter(MVKFenceSitter* fenceSitter);

	/** Removes the specified fence sitter. */
	void removeSitter(MVKFenceSitter* fenceSitter);

	/** Signals this fence. Notifies all waiting fence sitters. */
	void signal();

	/** Rremoves all fence sitters and resets this fence back to unsignaled state again. */
	void reset();

	/** Returns whether this fence has been signaled and not reset. */
	bool getIsSignaled();

	
#pragma mark Construction

    MVKFence(MVKDevice* device, const VkFenceCreateInfo* pCreateInfo) :
		MVKVulkanAPIDeviceObject(device), _isSignaled(mvkAreAllFlagsEnabled(pCreateInfo->flags, VK_FENCE_CREATE_SIGNALED_BIT)) {}

protected:
	void propogateDebugName() override {}
	void notifySitters();

	std::mutex _lock;
	std::unordered_set<MVKFenceSitter*> _fenceSitters;
	bool _isSignaled;
};


#pragma mark -
#pragma mark MVKFenceSitter

/** An object that responds to signals from MVKFences. */
class MVKFenceSitter : public MVKBaseObject {

public:

	/** This is a temporarily instantiated helper class. */
	MVKVulkanAPIObject* getVulkanAPIObject() override { return nullptr; }

	/**
	 * If this instance has been configured to wait for fences, blocks processing on the
	 * current thread until any or all of the fences that this instance is waiting for are
	 * signaled, or until the specified timeout in nanoseconds expires. If this instance
	 * has not been configured to wait for fences, this function immediately returns true.
	 *
	 * If timeout is the special value UINT64_MAX the timeout is treated as infinite.
	 *
	 * Returns true if the required fences were triggered, or false if the timeout interval expired.
	 */
	bool wait(uint64_t timeout = UINT64_MAX) { return _blocker.wait(timeout); }


#pragma mark Construction

	MVKFenceSitter(bool waitAll) : _blocker(waitAll, 0) {}

private:
	friend class MVKFence;

	void awaitFence(MVKFence* fence) { _blocker.reserve(); }
	void fenceSignaled(MVKFence* fence) { _blocker.release(); }

	MVKSemaphoreImpl _blocker;
};


#pragma mark -
#pragma mark MVKEvent

/** Abstract class that represents a Vulkan event. */
class MVKEvent : public MVKVulkanAPIDeviceObject {

public:

	/** Returns the Vulkan type of this object. */
	VkObjectType getVkObjectType() override { return VK_OBJECT_TYPE_EVENT; }

	/** Returns the debug report object type of this object. */
	VkDebugReportObjectTypeEXT getVkDebugReportObjectType() override { return VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT; }

	/** Returns whether this event is set. */
	virtual bool isSet() = 0;

	/** Sets the signal status. */
	virtual void signal(bool status) = 0;

	/** Encodes an operation to signal the event with a status. */
	virtual void encodeSignal(id<MTLCommandBuffer> mtlCmdBuff, bool status) = 0;

	/** Encodes an operation to block command buffer operation until this event is signaled. */
	virtual void encodeWait(id<MTLCommandBuffer> mtlCmdBuff) = 0;


#pragma mark Construction

	MVKEvent(MVKDevice* device, const VkEventCreateInfo* pCreateInfo) : MVKVulkanAPIDeviceObject(device) {}

protected:
	void propogateDebugName() override {}

};


#pragma mark -
#pragma mark MVKEventNative

/** An MVKEvent that uses native MTLSharedEvent to provide VkEvent functionality. */
class MVKEventNative : public MVKEvent {

public:
	bool isSet() override;
	void signal(bool status) override;
	void encodeSignal(id<MTLCommandBuffer> mtlCmdBuff, bool status) override;
	void encodeWait(id<MTLCommandBuffer> mtlCmdBuff) override;

	MVKEventNative(MVKDevice* device, const VkEventCreateInfo* pCreateInfo);

	~MVKEventNative() override;

protected:
	id<MTLSharedEvent> _mtlEvent;
};


#pragma mark -
#pragma mark MVKEventEmulated

/** An MVKEvent that uses CPU synchronization to provide VkEvent functionality. */
class MVKEventEmulated : public MVKEvent {

public:
	bool isSet() override;
	void signal(bool status) override;
	void encodeSignal(id<MTLCommandBuffer> mtlCmdBuff, bool status) override;
	void encodeWait(id<MTLCommandBuffer> mtlCmdBuff) override;

	MVKEventEmulated(MVKDevice* device, const VkEventCreateInfo* pCreateInfo);

protected:
	MVKSemaphoreImpl _blocker;
	bool _inlineSignalStatus;
};


#pragma mark -
#pragma mark Support functions

/** Resets the specified fences. */
VkResult mvkResetFences(uint32_t fenceCount, const VkFence* pFences);

/** 
 * Blocks the current thread until any or all of the specified 
 * fences have been signaled, or the specified timeout occurs.
 */
VkResult mvkWaitForFences(MVKDevice* device,
						  uint32_t fenceCount,
						  const VkFence* pFences,
						  VkBool32 waitAll,
						  uint64_t timeout = UINT64_MAX);


#pragma mark -
#pragma mark MVKMetalCompiler

/**
 * Abstract class that creates Metal objects that require compilation, such as
 * MTLShaderLibrary, MTLFunction, MTLRenderPipelineState and MTLComputePipelineState.
 *
 * Instances of this class are one-shot, and can only be used for a single compilation.
 */
class MVKMetalCompiler : public MVKBaseObject {

public:

	/** Returns the Vulkan API opaque object controlling this object. */
	MVKVulkanAPIObject* getVulkanAPIObject() override { return _owner->getVulkanAPIObject(); };

	/** If this object is waiting for compilation to complete, deletion will be deferred until then. */
	void destroy() override;


#pragma mark Construction

	MVKMetalCompiler(MVKVulkanAPIDeviceObject* owner) : _owner(owner) {}

	~MVKMetalCompiler() override;

protected:
	void compile(std::unique_lock<std::mutex>& lock, dispatch_block_t block);
	virtual void handleError();
	bool endCompile(NSError* compileError);
	bool markDestroyed();

	MVKVulkanAPIDeviceObject* _owner;
	NSError* _compileError = nil;
	uint64_t _startTime = 0;
	bool _isCompileDone = false;
	bool _isDestroyed = false;
	std::mutex _completionLock;
	std::condition_variable _blocker;
	std::string _compilerType = "Unknown";
	MVKPerformanceTracker* _pPerformanceTracker = nullptr;
};
