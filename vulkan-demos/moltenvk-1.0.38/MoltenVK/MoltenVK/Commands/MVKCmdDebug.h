/*
 * MVKCmdDebug.h
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

#include "MVKCommand.h"

#import <Foundation/NSString.h>


#pragma mark -
#pragma mark MVKCmdDebugMarker

/**Abstract Vulkan class to support debug markers. */
class MVKCmdDebugMarker : public MVKCommand {

public:
	void setContent(const char* pMarkerName, const float color[4]);

    MVKCmdDebugMarker(MVKCommandTypePool<MVKCmdDebugMarker>* pool);

	~MVKCmdDebugMarker() override;

protected:
	NSString* _markerName = nil;
};


#pragma mark -
#pragma mark MVKCmdDebugMarkerBegin

/** Vulkan command to begin a marker region into the command buffer. */
class MVKCmdDebugMarkerBegin : public MVKCmdDebugMarker {

public:
	void encode(MVKCommandEncoder* cmdEncoder) override;

	MVKCmdDebugMarkerBegin(MVKCommandTypePool<MVKCmdDebugMarkerBegin>* pool);
};


#pragma mark -
#pragma mark MVKCmdDebugMarkerEnd

/** Vulkan command to end an open marker region in the command buffer. */
class MVKCmdDebugMarkerEnd : public MVKCommand {

public:
	void encode(MVKCommandEncoder* cmdEncoder) override;

	MVKCmdDebugMarkerEnd(MVKCommandTypePool<MVKCmdDebugMarkerEnd>* pool);
};


#pragma mark -
#pragma mark MVKCmdDebugMarkerInsert

	/** Vulkan command to insert a debug marker into the command encoder. */
	class MVKCmdDebugMarkerInsert : public MVKCmdDebugMarker {

	public:
		void encode(MVKCommandEncoder* cmdEncoder) override;

		MVKCmdDebugMarkerInsert(MVKCommandTypePool<MVKCmdDebugMarkerInsert>* pool);
	};


#pragma mark -
#pragma mark Command creation functions

void mvkCmdDebugMarkerBegin(MVKCommandBuffer* cmdBuff, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo);

void mvkCmdDebugMarkerEnd(MVKCommandBuffer* cmdBuff);

void mvkCmdDebugMarkerInsert(MVKCommandBuffer* cmdBuff, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo);

void mvkCmdBeginDebugUtilsLabel(MVKCommandBuffer* cmdBuff, const VkDebugUtilsLabelEXT* pLabelInfo);

void mvkCmdEndDebugUtilsLabel(MVKCommandBuffer* cmdBuff);

void mvkCmdInsertDebugUtilsLabel(MVKCommandBuffer* cmdBuff, const VkDebugUtilsLabelEXT* pLabelInfo);


#pragma mark -
#pragma mark Support functions

void mvkPushDebugGroup(id<MTLCommandBuffer> mtlCmdBuffer, NSString* name);

void mvkPopDebugGroup(id<MTLCommandBuffer> mtlCmdBuffer);

