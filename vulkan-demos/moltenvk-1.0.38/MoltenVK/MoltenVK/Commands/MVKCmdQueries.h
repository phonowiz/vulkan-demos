/*
 * MVKCmdQueries.h
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

class MVKQueryPool;


#pragma mark -
#pragma mark MVKCmdQuery

/** Abstract Vulkan command to manage queries. */
class MVKCmdQuery : public MVKCommand {

public:
    void setContent(VkQueryPool queryPool, uint32_t query);

    MVKCmdQuery(MVKCommandTypePool<MVKCommand>* pool);

protected:
    MVKQueryPool* _queryPool;
    uint32_t _query;
};


#pragma mark -
#pragma mark MVKCmdBeginQuery

/** Vulkan command to begin a query. */
class MVKCmdBeginQuery : public MVKCmdQuery {

public:
    void setContent(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);

    void added(MVKCommandBuffer* cmdBuffer) override;

    void encode(MVKCommandEncoder* cmdEncoder) override;

    MVKCmdBeginQuery(MVKCommandTypePool<MVKCmdBeginQuery>* pool);

protected:
    VkQueryControlFlags _flags;
};


#pragma mark -
#pragma mark MVKCmdEndQuery

/** Vulkan command to end a query. */
class MVKCmdEndQuery : public MVKCmdQuery {

public:
    void encode(MVKCommandEncoder* cmdEncoder) override;

    MVKCmdEndQuery(MVKCommandTypePool<MVKCmdEndQuery>* pool);
};


#pragma mark -
#pragma mark MVKCmdWriteTimestamp

/** Vulkan command to write a timestamp. */
class MVKCmdWriteTimestamp : public MVKCmdQuery {

public:
    void setContent(VkPipelineStageFlagBits pipelineStage,
                    VkQueryPool queryPool,
                    uint32_t query);

    void encode(MVKCommandEncoder* cmdEncoder) override;

    MVKCmdWriteTimestamp(MVKCommandTypePool<MVKCmdWriteTimestamp>* pool);

protected:
    VkPipelineStageFlagBits _pipelineStage;
};


#pragma mark -
#pragma mark MVKCmdResetQueryPool

/** Vulkan command to reset the results in a query pool. */
class MVKCmdResetQueryPool : public MVKCmdQuery {

public:
    void setContent(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);

    void encode(MVKCommandEncoder* cmdEncoder) override;

    MVKCmdResetQueryPool(MVKCommandTypePool<MVKCmdResetQueryPool>* pool);

protected:
    uint32_t _queryCount;
};


#pragma mark -
#pragma mark MVKCmdCopyQueryPoolResults

/** Vulkan command to reset the results in a query pool. */
class MVKCmdCopyQueryPoolResults : public MVKCmdQuery {

public:
    void setContent(VkQueryPool queryPool,
                    uint32_t firstQuery,
                    uint32_t queryCount,
                    VkBuffer destBuffer,
                    VkDeviceSize destOffset,
                    VkDeviceSize destStride,
                    VkQueryResultFlags flags);

    void encode(MVKCommandEncoder* cmdEncoder) override;

    MVKCmdCopyQueryPoolResults(MVKCommandTypePool<MVKCmdCopyQueryPoolResults>* pool);

protected:
    uint32_t _queryCount;
    MVKBuffer* _destBuffer;
    VkDeviceSize _destOffset;
    VkDeviceSize _destStride;
    VkQueryResultFlags _flags;
};


#pragma mark -
#pragma mark Command creation functions

/** Adds a begin query command to the specified command buffer. */
void mvkCmdBeginQuery(MVKCommandBuffer* cmdBuff,
                      VkQueryPool queryPool,
                      uint32_t query,
                      VkQueryControlFlags flags);

/** Adds an end query command to the specified command buffer. */
void mvkCmdEndQuery(MVKCommandBuffer* cmdBuff,
                    VkQueryPool queryPool,
                    uint32_t query);

/** Adds a write timestamp command to the specified command buffer. */
void mvkCmdWriteTimestamp(MVKCommandBuffer* cmdBuff,
						  VkPipelineStageFlagBits pipelineStage,
						  VkQueryPool queryPool,
						  uint32_t query);

/** Adds a reset query pool command to the specified command buffer. */
void mvkCmdResetQueryPool(MVKCommandBuffer* cmdBuff,
                          VkQueryPool queryPool,
                          uint32_t firstQuery,
                          uint32_t queryCount);

/** Adds a copy query pool results command to the specified command buffer. */
void mvkCmdCopyQueryPoolResults(MVKCommandBuffer* cmdBuff,
                                VkQueryPool queryPool,
                                uint32_t firstQuery,
                                uint32_t queryCount,
                                VkBuffer destBuffer,
                                VkDeviceSize destOffset,
                                VkDeviceSize destStride,
                                VkQueryResultFlags flags);


