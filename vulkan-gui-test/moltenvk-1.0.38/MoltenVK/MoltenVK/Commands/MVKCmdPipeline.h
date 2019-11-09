/*
 * MVKCmdPipeline.h
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
#include "MVKVector.h"

class MVKCommandBuffer;
class MVKPipeline;
class MVKPipelineLayout;
class MVKDescriptorSet;
class MVKDescriptorUpdateTemplate;


#pragma mark -
#pragma mark MVKCmdPipelineBarrier

/** Represents an abstract Vulkan command to add a pipeline barrier. */
class MVKCmdPipelineBarrier : public MVKCommand {

public:
	void setContent(VkPipelineStageFlags srcStageMask,
					VkPipelineStageFlags dstStageMask,
					VkDependencyFlags dependencyFlags,
					uint32_t memoryBarrierCount,
					const VkMemoryBarrier* pMemoryBarriers,
					uint32_t bufferMemoryBarrierCount,
					const VkBufferMemoryBarrier* pBufferMemoryBarriers,
					uint32_t imageMemoryBarrierCount,
					const VkImageMemoryBarrier* pImageMemoryBarriers);

	void encode(MVKCommandEncoder* cmdEncoder) override;

	MVKCmdPipelineBarrier(MVKCommandTypePool<MVKCmdPipelineBarrier>* pool);

private:
	VkPipelineStageFlags _srcStageMask;
	VkPipelineStageFlags _dstStageMask;
	VkDependencyFlags _dependencyFlags;
	MVKVectorInline<VkMemoryBarrier, 4> _memoryBarriers;
	MVKVectorInline<VkBufferMemoryBarrier, 4> _bufferMemoryBarriers;
	MVKVectorInline<VkImageMemoryBarrier, 4> _imageMemoryBarriers;
};


#pragma mark -
#pragma mark MVKCmdBindPipeline

/** Vulkan command to bind the pipeline state. */
class MVKCmdBindPipeline : public MVKCommand {

public:
	void setContent(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);

	void encode(MVKCommandEncoder* cmdEncoder) override;

	MVKCmdBindPipeline(MVKCommandTypePool<MVKCmdBindPipeline>* pool);

	bool isTessellationPipeline();

private:
	VkPipelineBindPoint _bindPoint;
	MVKPipeline* _pipeline;

};


#pragma mark -
#pragma mark MVKCmdBindDescriptorSets

/** Vulkan command to bind descriptor sets. */
class MVKCmdBindDescriptorSets : public MVKCommand {

public:
	void setContent(VkPipelineBindPoint pipelineBindPoint,
					VkPipelineLayout layout,
					uint32_t firstSet,
					uint32_t setCount,
					const VkDescriptorSet* pDescriptorSets,
					uint32_t dynamicOffsetCount,
					const uint32_t* pDynamicOffsets);

	void encode(MVKCommandEncoder* cmdEncoder) override;

	MVKCmdBindDescriptorSets(MVKCommandTypePool<MVKCmdBindDescriptorSets>* pool);

private:
	VkPipelineBindPoint _pipelineBindPoint;
	MVKPipelineLayout* _pipelineLayout;
	MVKVectorInline<MVKDescriptorSet*, 8> _descriptorSets;
	MVKVectorInline<uint32_t, 8>          _dynamicOffsets;
	uint32_t _firstSet;
};


#pragma mark -
#pragma mark MVKCmdPushConstants

/** Vulkan command to bind push constants. */
class MVKCmdPushConstants : public MVKCommand {

public:
	void setContent(VkPipelineLayout layout,
					VkShaderStageFlags stageFlags,
					uint32_t offset,
					uint32_t size,
					const void* pValues);

	void encode(MVKCommandEncoder* cmdEncoder) override;

	MVKCmdPushConstants(MVKCommandTypePool<MVKCmdPushConstants>* pool);

private:
	MVKPipelineLayout* _pipelineLayout;
	VkShaderStageFlags _stageFlags;
	uint32_t _offset;
	MVKVectorInline<char, 128> _pushConstants;
};


#pragma mark -
#pragma mark MVKCmdPushDescriptorSet

/** Vulkan command to update a descriptor set. */
class MVKCmdPushDescriptorSet : public MVKCommand {

public:
	void setContent(VkPipelineBindPoint pipelineBindPoint,
					VkPipelineLayout layout,
					uint32_t set,
					uint32_t descriptorWriteCount,
					const VkWriteDescriptorSet* pDescriptorWrites);

	void encode(MVKCommandEncoder* cmdEncoder) override;

	MVKCmdPushDescriptorSet(MVKCommandTypePool<MVKCmdPushDescriptorSet>* pool);

	~MVKCmdPushDescriptorSet() override;

private:
	void clearDescriptorWrites();

	VkPipelineBindPoint _pipelineBindPoint;
	MVKPipelineLayout* _pipelineLayout;
	MVKVectorInline<VkWriteDescriptorSet, 8> _descriptorWrites;
	uint32_t _set;
};


#pragma mark -
#pragma mark MVKCmdPushDescriptorSetWithTemplate

/** Vulkan command to update a descriptor set from a template. */
class MVKCmdPushDescriptorSetWithTemplate : public MVKCommand {

public:
	void setContent(VkDescriptorUpdateTemplateKHR descUpdateTemplate,
					VkPipelineLayout layout,
					uint32_t set,
					const void* pData);

	void encode(MVKCommandEncoder* cmdEncoder) override;

	MVKCmdPushDescriptorSetWithTemplate(MVKCommandTypePool<MVKCmdPushDescriptorSetWithTemplate>* pool);

	~MVKCmdPushDescriptorSetWithTemplate() override;

private:
	MVKDescriptorUpdateTemplate* _descUpdateTemplate;
	MVKPipelineLayout* _pipelineLayout;
	uint32_t _set;
	void* _pData = nullptr;
};


#pragma mark -
#pragma mark MVKCmdSetResetEvent

/** Vulkan command to set or reset an event. */
class MVKCmdSetResetEvent : public MVKCommand {

public:
	void setContent(VkEvent event, VkPipelineStageFlags stageMask, bool status);

	void encode(MVKCommandEncoder* cmdEncoder) override;

	MVKCmdSetResetEvent(MVKCommandTypePool<MVKCmdSetResetEvent>* pool);

private:
	MVKEvent* _mvkEvent;
	bool _status;

};


#pragma mark -
#pragma mark MVKCmdWaitEvents

/** Vulkan command to wait for an event to be signaled. */
class MVKCmdWaitEvents : public MVKCommand {

public:
	void setContent(uint32_t eventCount,
					const VkEvent* pEvents,
					VkPipelineStageFlags srcStageMask,
					VkPipelineStageFlags dstStageMask,
					uint32_t memoryBarrierCount,
					const VkMemoryBarrier* pMemoryBarriers,
					uint32_t bufferMemoryBarrierCount,
					const VkBufferMemoryBarrier* pBufferMemoryBarriers,
					uint32_t imageMemoryBarrierCount,
					const VkImageMemoryBarrier* pImageMemoryBarriers);

	void encode(MVKCommandEncoder* cmdEncoder) override;

	MVKCmdWaitEvents(MVKCommandTypePool<MVKCmdWaitEvents>* pool);

private:
	MVKVectorInline<MVKEvent*, 4> _mvkEvents;

};


#pragma mark -
#pragma mark Command creation functions

/** Adds commands to the specified command buffer that insert the specified pipeline barriers. */
void mvkCmdPipelineBarrier(MVKCommandBuffer* cmdBuff,
						   VkPipelineStageFlags srcStageMask,
						   VkPipelineStageFlags dstStageMask,
						   VkDependencyFlags dependencyFlags,
						   uint32_t memoryBarrierCount,
						   const VkMemoryBarrier* pMemoryBarriers,
						   uint32_t bufferMemoryBarrierCount,
						   const VkBufferMemoryBarrier* pBufferMemoryBarriers,
						   uint32_t imageMemoryBarrierCount,
						   const VkImageMemoryBarrier* pImageMemoryBarriers);

/** Adds a command to the specified command buffer that binds the specified pipeline. */
void mvkCmdBindPipeline(MVKCommandBuffer* cmdBuff,
						VkPipelineBindPoint pipelineBindPoint,
						VkPipeline pipeline);

/** Adds commands to the specified command buffer that insert the specified descriptor sets. */
void mvkCmdBindDescriptorSets(MVKCommandBuffer* cmdBuff,
							  VkPipelineBindPoint pipelineBindPoint,
							  VkPipelineLayout layout,
							  uint32_t firstSet,
							  uint32_t setCount,
							  const VkDescriptorSet* pDescriptorSets,
							  uint32_t dynamicOffsetCount,
							  const uint32_t* pDynamicOffsets);

/** Adds a vertex bind command to the specified command buffer. */
void mvkCmdPushConstants(MVKCommandBuffer* cmdBuff,
						 VkPipelineLayout layout,
						 VkShaderStageFlags stageFlags,
						 uint32_t offset,
						 uint32_t size,
						 const void* pValues);

/** Adds commands to the specified command buffer that update the specified descriptor set. */
void mvkCmdPushDescriptorSet(MVKCommandBuffer* cmdBuff,
							 VkPipelineBindPoint pipelineBindPoint,
							 VkPipelineLayout layout,
							 uint32_t set,
							 uint32_t descriptorWriteCount,
							 const VkWriteDescriptorSet* pDescriptorWrites);

/** Adds commands to the specified command buffer that update the specified descriptor set from the given template. */
void mvkCmdPushDescriptorSetWithTemplate(MVKCommandBuffer* cmdBuff,
										 VkDescriptorUpdateTemplateKHR descUpdateTemplate,
										 VkPipelineLayout layout,
										 uint32_t set,
										 const void* pData);

/** Adds a set event command to the specified command buffer. */
void mvkCmdSetEvent(MVKCommandBuffer* cmdBuff,
					VkEvent event,
					VkPipelineStageFlags stageMask);

/** Adds a reset event command to the specified command buffer. */
void mvkCmdResetEvent(MVKCommandBuffer* cmdBuff,
					  VkEvent event,
					  VkPipelineStageFlags stageMask);


/** Adds a wait events command to the specified command buffer. */
void mvkCmdWaitEvents(MVKCommandBuffer* cmdBuff,
					  uint32_t eventCount,
					  const VkEvent* pEvents,
					  VkPipelineStageFlags srcStageMask,
					  VkPipelineStageFlags dstStageMask,
					  uint32_t memoryBarrierCount,
					  const VkMemoryBarrier* pMemoryBarriers,
					  uint32_t bufferMemoryBarrierCount,
					  const VkBufferMemoryBarrier* pBufferMemoryBarriers,
					  uint32_t imageMemoryBarrierCount,
					  const VkImageMemoryBarrier* pImageMemoryBarriers);

/** Indicates that following commands are to be recorded only for the devices in the given device mask. */
void mvkCmdSetDeviceMask(MVKCommandBuffer* cmdBuff, uint32_t deviceMask);
