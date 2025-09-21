#include "stdlib.h"
#include "mem.h"

POOL_INFO *pool_info = NULL;

/**
 * 创建一个内存池
 * @param block_count 内存块的数量
 * @param block_size 每个内存块的大小
 */
void create_pool(size_t block_count, size_t block_size)
{
    // 如果内存池已经存在，直接返回
    if (pool_info != NULL)
        return;

    // 分配内存池的总空间
    void *pool;
    pool = malloc(block_count * block_size);
    if (pool == NULL)
        return;

    // 初始化内存池信息结构体
    pool_info = (POOL_INFO *)pool;
    *pool_info = (POOL_INFO){pool, block_size, block_count * block_size, block_count, 0, NULL, NULL};
    // 初始化内存块信息数组
    BLOCK_INFO *block_info = (BLOCK_INFO *)(pool + sizeof(POOL_INFO));

    // 遍历所有内存块，初始化状态和地址
    for (size_t i = 0; i < pool_info->block_count; i++)
    {
        block_info[i] = (BLOCK_INFO){FREE, pool + block_size * i};
    }

    // 计算用于存储池信息和块信息的系统块数量
    size_t pool_info_size = (size_t)((sizeof(POOL_INFO) + sizeof(BLOCK_INFO) * pool_info->block_count) / pool_info->block_size) + 1;

    // 将前pool_info_size个块标记为系统块
    for (size_t i = 0; i < pool_info_size; i++)
    {
        block_info[i].status = SYSTEM;
    }

    // 设置块信息数组的指针
    pool_info->block_infos = block_info;

    // 初始化分配信息数组
    ALLOC_INFO *alloc_infos = (ALLOC_INFO *)pool_info->block_infos[pool_info_size].block;
    pool_info->block_infos[pool_info_size].status = SYSTEM;
    pool_info->alloc_infos = alloc_infos;
}

/**
 * 释放内存池资源
 * 该函数用于释放内存池分配的内存，并将pool_info指针置为NULL
 */
void free_pool()
{
    // 检查pool_info是否为NULL，如果是则直接返回
    if (pool_info == NULL)
        return;

    // 释放内存池中分配的内存
    free(pool_info->pool);
    // 将pool_info指针置为NULL，防止悬空指针
    pool_info = NULL;
}

/**
 * 根据块ID获取对应的内存块
 * @param id 要获取的内存块的ID
 * @return 返回对应ID的内存块指针，如果ID无效或内存池信息不存在则返回NULL
 */
void *get_block(size_t id)
{
    // 检查内存池信息是否存在或ID是否超出范围
    if (pool_info == NULL || id >= pool_info->block_count)
        return NULL;

    // 返回对应ID的内存块指针
    return pool_info->block_infos[id].block;
}

/**
 * 获取指针所在的内存块ID
 * @param ptr 要查询的内存指针
 * @return 内存块ID，如果指针不在任何有效块内则返回-1
 */
size_t get_block_id(void *ptr)
{
    // 检查池信息是否存在，指针是否为空，以及指针是否在内存池范围内
    if (pool_info == NULL || ptr == NULL || ptr < pool_info->pool || ptr >= pool_info->pool + pool_info->pool_size)
        return -1;

    // 遍历所有内存块，查找指针所在的块
    for (size_t i = 0; i < pool_info->block_count; i++)
    {
        // 检查指针是否在当前块范围内
        if (ptr >= pool_info->block_infos[i].block && ptr < pool_info->block_infos[i].block + pool_info->block_size)
            return i; // 返回找到的块ID
    }

    // 未找到匹配的块，返回-1
    return -1;
}

/**
 * 分配指定大小的内存块
 * @param size 需要分配的内存大小
 * @return 成功则返回分配的内存指针，失败返回NULL
 */
void *alloc(size_t size)
{
    // 计算需要的块数量，向上取整
    size_t block_size = size / pool_info->block_size + 1;
    // 检查池中是否有足够的可用块
    if (pool_info->block_count < block_size)
    {
        return NULL;
    }

    // 从后向前遍历查找连续的空闲块
    for (size_t i = pool_info->block_count - 1; i >= 0; i--)
    {
        // 找到空闲块
        if (pool_info->block_infos[i].status == false)
        {
            bool flag = FREE;           // 标记块的状态
            void *first_pointer = NULL; // 记录第一个块的指针
            size_t first_block = i;     // 记录第一个块的索引
            // 向前检查连续的空闲块
            for (size_t j = i; j > i - block_size && j >= 0; j--)
            {
                flag = pool_info->block_infos[j].status;
                first_pointer = pool_info->block_infos[j].block;
                first_block = j;
                // 如果遇到已分配的块，停止检查
                if (flag != FREE)
                {
                    i = j - 1;
                    break;
                }
            }

            // 如果不是连续的空闲块，继续查找
            if (flag != FREE)
            {
                continue;
            }

            // 标记这些块为已分配
            for (size_t j = first_block; j <= first_block + block_size; j++)
            {
                pool_info->block_infos[j].status = ALLOCED;
            }

            // 注册分配信息
            if (register_alloc(first_pointer, first_block, first_block + block_size))
            {
                // 如果注册失败，回滚分配状态
                for (size_t j = first_block; j <= first_block + block_size; j++)
                {
                    pool_info->block_infos[j].status = FREE;
                }
                return NULL;
            }

            // 返回分配的内存指针
            return first_pointer;
        }
    }
    // 没有找到合适的内存块，返回NULL
    return NULL;
}

/**
 * 释放已分配的内存块
 * @param ptr 指向要释放的内存块的指针
 */
void free(void *ptr)
{
    // 检查指针是否为空或内存池信息是否为空
    if (ptr == NULL || pool_info == NULL)
        return;

    bool found = false;            // 标记是否找到对应的内存块
    ALLOC_INFO *alloc_info = NULL; // 存储找到的分配信息

    // 遍历所有分配信息，查找要释放的内存块
    for (size_t i = 0; i < pool_info->alloc_count; i++)
    {
        // 检查当前分配信息是否匹配目标指针且正在使用中
        if (pool_info->alloc_infos[i].pointer == ptr && pool_info->alloc_infos[i].using)
        {
            found = true;
            alloc_info = pool_info->alloc_infos + i;
            break;
        }
    }

    // 如果未找到对应的内存块，直接返回
    if (!found)
        return;

    // 将该内存块占用的所有块状态设置为FREE
    for (size_t i = alloc_info->start_block; i <= alloc_info->end_block; i++)
    {
        pool_info->block_infos[i].status = FREE;
    }

    // 标记该分配信息为未使用状态
    alloc_info->using = false;
}

/**
 * @brief 注册内存分配信息
 *
 * 该函数用于在内存池中注册一个新的内存分配信息。它会将指定的内存块标记为系统使用，
 * 并记录分配的详细信息。
 *
 * @param ptr 指向要分配的内存块的指针
 * @param start_block 起始块号
 * @param end_block 结束块号
 * @return bool 成功返回false，失败返回true
 */
bool register_alloc(void *ptr, size_t start_block, size_t end_block)
{
    // 检查参数有效性：内存池信息是否为空、指针是否为空、块号是否有效
    if (pool_info == NULL || ptr == NULL || start_block <= 0 || end_block > pool_info->block_count)
        return true;

    // 创建分配信息结构体
    ALLOC_INFO alloc_info = (ALLOC_INFO){ptr, start_block, end_block, true};
    // 获取下一个可用的分配信息存储位置
    ALLOC_INFO *alloc_info_ptr = pool_info->alloc_infos + pool_info->alloc_count;

    // 检查目标块是否已被分配
    if (pool_info->block_infos[get_block_id(alloc_info_ptr)].status == ALLOCED)
    {
        return true;
    }

    // 将目标块标记为系统使用
    pool_info->block_infos[get_block_id(alloc_info_ptr)].status = SYSTEM;
    // 存储分配信息
    *alloc_info_ptr = alloc_info;
    // 增加分配计数
    pool_info->alloc_count++;
    return false;
}

/*
 * memclear - 将指定内存区域清零
 * @ptr: 指向要清零的内存区域的指针
 * @size: 要清零的内存区域的大小（字节）
 *
 * 该函数将从ptr开始的size个字节的内存内容设置为零。
 * 如果ptr为NULL，函数直接返回，不做任何操作。
 */
void memclear(void *ptr, size_t size)
{
    // 检查指针是否为空，空指针直接返回
    if (ptr == NULL)
        return;

    // 逐字节将内存内容设置为零
    for (size_t i = 0; i < size; i++)
    {
        ((char *)ptr)[i] = 0;
    }
}
