#include "stdio.h"
#include "includes/mem.h"

const size_t BLOCK_SIZE = 0x400;  // 1KB
const size_t BLOCK_COUNT = 0x400; // 1MB

/**
 * 主函数 - 演示内存池的创建、分配、使用、释放和销毁
 */
int main()
{
    // 创建内存池，每个块大小为BLOCK_SIZE，共BLOCK_COUNT个块
    create_pool(BLOCK_SIZE, BLOCK_COUNT);

    // 计算可以存储的int元素数量
    size_t size = 0x300 * BLOCK_SIZE / sizeof(int);
    // 分配内存空间，大小为0x300个BLOCK_SIZE
    int *ptr1 = (int *)alloc(0x300 * BLOCK_SIZE);

    // 使用循环将0到size-1的值依次赋给ptr1指向的数组元素
    for (size_t i = 0; i < size; i++) ptr1[i] = i;

    // 打印ptr1数组中的所有元素
    for (size_t i = 0; i < size; i++) printf("%d\n", ptr1[i]);

    // 释放之前分配的内存
    free(ptr1);

    // 销毁整个内存池
    free_pool();

    // 程序正常结束
    return 0;
}