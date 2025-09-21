#pragma once
#ifndef MEM_H
#define MEM_H
#include "stddef.h"
#include "stdbool.h"

// 定义内存块状态枚举
typedef enum BLOCK_STATUS{
    FREE,    // 空闲状态
    ALLOCED, // 已分配状态
    SYSTEM,  // 系统状态
} BLOCK_STATUS;

// 定义内存块信息结构体
typedef struct BLOCK_INFO
{
    BLOCK_STATUS status;  // 内存块状态
    void *block;          // 指向内存块的指针
} BLOCK_INFO;

// 定义分配信息结构体
typedef struct ALLOC_INFO
{
    void *pointer;       // 指向分配的内存
    size_t start_block;  // 起始块ID
    size_t end_block;    // 结束块ID
    bool using;          // 是否正在使用
} ALLOC_INFO;

// 定义内存池信息结构体
typedef struct POOL_INFO
{
    void *pool;          // 内存池指针
    size_t block_size;   // 每个块的大小
    size_t pool_size;    // 内存池总大小
    size_t block_count;  // 块的总数
    size_t alloc_count;  // 已分配块的数量
    BLOCK_INFO *block_infos;  // 块信息数组
    ALLOC_INFO *alloc_infos;  // 分配信息数组
} POOL_INFO;

// 全局内存池信息指针
extern POOL_INFO *pool_info;



// 函数声明
void create_pool(size_t block_count, size_t block_size);  // 创建内存池
void free_pool();  // 释放内存池

void *get_block(size_t id);  // 获取指定ID的内存块
size_t get_block_id(void *ptr);  // 获取指针所在的块ID

void *alloc(size_t size);  // 分配内存
void free(void *ptr);  // 释放内存

bool register_alloc(void *ptr, size_t start_block, size_t end_block);  // 注册分配信息

void memclear(void *ptr, size_t size);  // 清除内存

#endif
