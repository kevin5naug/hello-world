/*
 * Block structure
 *
 *
 * An allocated block
 *
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- current
 * |  Size of previous block                               |0|0|0|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Size of current block                                |0|0|0|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- user
 * .                                                               .
 * .  User data                                                    .
 * .                                                               .
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- next
 * |  Size of current block                                |0|0|0|F|  (if it is the epilogue block)
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- (brk)*
 *
 * 
 * Since we want the returned pointer of mm_malloc to be a multiple of 16 bytes, 
 * we have to remove footnote completely and put information in the "size of previous block". 
 * Otherwise, we have to allocate 16 bytes for one header and
 * another 16 bytes for the footnote, a waste of memory.
 * Since last 4 bits of size will be 0, the last bit is used for
 * indicating whether the corresponding block is free. (1 if free)
 * 
 * An free block
 *
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- current
 * |  Size of previous block                               |0|0|0|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Size of current block                                |0|0|0|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Address of left child                                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Address of right child                                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Address of parent                                            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Garbage                                                    |R|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * .                                                               .
 * .  Garbage                                                      .
 * .                                                               .
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- next
 * |  Size of current block                                 |0|0|0|F| (if it is the epilogue block)
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- (brk)*
 * 
 * For a free block, in addition to the size of previous block and current block, 
 * we have to add the "children" and "parent" of the block as a node in a Red-black Tree
 * for malloc, free and other functions. Color information is stored in the last bit 
 * after the "parent" information (1 if red). 
 * Therefore, in order to store free block in the tree, the block should be at least 48-byte 
 * (MIN_BLOCK_SIZE) including the header, but a free block which is smaller than MIN_BLOCK_SIZE 
 * can still happen when we split a large block into two smaller one for a malloc request. 
 * When such incident happens, we have to check whether the "reminder" is larger than the MIN_BLOCK_SIZE
 * or not. If not, it will be kept off the record and treated as fragmentation.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "csoteam",
    /* First member's full name */
    "Yixing Guan",
    /* First member's github username*/
    "kevin5naug",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's github username (leave blank if none) */
    ""
};


#define WSIZE 8
#define DSIZE 16
#define CHUNKSIZE (1<<12)

#define HEADER_SIZE 16 
#define MIN_BLOCK_SIZE 48
#define FAIL ((void*)-1)

/*pointer macros*/
/*Pack a size and freed bit into a word*/
#define PACK(size,free) ((size)|(free))

/*Read and write a word t address p*/
#define GET(p) (*(unsigned long *)(p))
#define PUT(p,val) (*(unsigned long *)(p)=(val))

/*Read the size and allocated fields from address p*/
#define GET_SIZE(p) (GET(p) & ~0xf)
#define GET_FREE(p) (GET(p) & 0x1)

/*Given the starting ptr p, read the size and allocated fields of the previous block*/
#define PREV_SIZE(p) (*(size_t*)(p))
#define PREV_SIZE_MASKED(p) (PREV_SIZE(p) & ~0xf)
#define PREV_FREE(p) (PREV_SIZE(p) & 0x1)

/*Given the starting ptr p, read the size and allocated fields of the current block*/
#define CUR_SIZE(p) (*(size_t*)((p)+8))
#define CUR_SIZE_MASKED(p) (CUR_SIZE(p) & ~0xf)
#define CUR_FREE(p) (CUR_SIZE(p) & 0x1)

/*Given the starting ptr p, compute address of the block ptr bp*/
#define USER_BLOCK(p) ((p)+HEADER_SIZE)

/*Given the starting ptr p and the size of the block sz, compute address of previous and next blocks*/
#define PREV_BLOCK(p,sz) ((p)-(sz))
#define NEXT_BLOCK(p,sz) ((p)+(sz))

/*Given the starting ptr p of a free block, determine whether it is tracked by a Red-black Tree or not*/
#define IS_IN_TREE(p) (CUR_SIZE_MASKED(p)>=MIN_BLOCK_SIZE)

/*The following macors are mainly for the free blocks*/
/*Given the starting ptr p, read the addresses of the left and right children of this free block in the Red-black Tree*/
#define LEFT_CHILD(p) (*(void**)((p)+16))
#define RIGHT_CHILD(p) (*(void**)((p)+24))

/*Given the starting ptr p, read the address of the parent of this free block in the Red-black Tree*/
#define TREE_PARENT(p) (*(void**)((p)+32))

/*Given the starting ptr p, read the color(Red/Black) from the corresponding field of the free block*/
#define IS_RED(p) ((*(int*)((p)+40)))

/*root and null node(the prologue and epilogue) of Red-black Tree*/
void *tree_root, *tree_null;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /*Allocate the initial heap area(the first MIN_BLOCK_SIZE bytes are for the prologue block, 
     * while the last WSIZE bytes are for the epilogue block)*/
    tree_null=mem_sbrk(WSIZE+MIN_BLOCK_SIZE);
    tree_root=tree_null;
    if(tree_root==FAIL){
	return -1;
    }
    /*Let the root node point to the null node*/
    LEFT_CHILD(tree_root)=tree_null;
    RIGHT_CHILD(tree_root)=tree_null;
    IS_RED(tree_root)=0;

    /*Set the initial block to be 'allocated' so it will never be coalesced.
     * Recorded by the last WSIZE bytes.*/
    PREV_SIZE(NEXT_BLOCK(tree_root, MIN_BLOCK_SIZE))=0;
    CUR_SIZE(tree_root)=MIN_BLOCK_SIZE;
    return 0;
}

/*The following functions of Red-black Tree are helper functions for the implementation of mm_malloc and mm_free. 
 * Note that in these functions, LEFT_CHILD(tree_root) always points to the TRUE Root of the Red-black Tree.*/

/*
 *tree_find - (Best-fit Policy) find the smallest free block larger than or equal to size.
 */
void *tree_find(size_t size){
    void *node=LEFT_CHILD(tree_root), *best=tree_null;
    
    while(node!=tree_null){
	if(CUR_SIZE_MASKED(node)<size){
	    node=RIGHT_CHILD(node);
	}else{
	    best=node;
	    node=LEFT_CHILD(node);
	}
    }
    return best;
}

/*
 * tree_find_exact - check whether a block is a node of the Red-black Tree or not.
 * */
int tree_find_exact(void *block){
    void *node=LEFT_CHILD(tree_root);
    
    while(node!=tree_null){
	if(node==block){
	    return 1;
	}else if(CUR_SIZE_MASKED(node)>CUR_SIZE_MASKED(block)){
	    node=LEFT_CHILD(node);
	}else if(CUR_SIZE_MASKED(node)==CUR_SIZE_MASKED(block)){
	    /*In consistency with the implementation of the insertion operation of Black-red Tree*/
	    if(node>block){
		node=LEFT_CHILD(node);
	    }else{
		node=RIGHT_CHILD(node);
	    }
	}else{
	    node=RIGHT_CHILD(node);
	}
    }
    return 0;
}

/*
 * tree_successor - locate the address of next node of node if we list all nodes of this tree in ascending order.
 */
void* tree_successor(void *node){
    void *succ, *left;
    
    succ=RIGHT_CHILD(node);
    if(succ!=tree_null){
        left=LEFT_CHILD(succ);
	while(left!=tree_null){
	    succ=left;
            left=LEFT_CHILD(succ);
	}
	return succ;
    }else{
	succ=TREE_PARENT(node);
	while(RIGHT_CHILD(succ)==node){
	    node=succ;
	    succ=TREE_PARENT(succ);
	}
	if(succ==tree_root){
	    return tree_null;
	}
	return succ;
    }
}

/*
 * branch_rotate_left -an operation affiliated to tree_fix, rotate node and children of node to the left
 * */
void branch_rotate_left(void *node){
    void *right;

    right=RIGHT_CHILD(node);
    RIGHT_CHILD(node)=LEFT_CHILD(right);
    if(LEFT_CHILD(right)!=tree_null)
        TREE_PARENT(LEFT_CHILD(right))=node;

    TREE_PARENT(right)=TREE_PARENT(node);
    if(node==LEFT_CHILD(TREE_PARENT(node))){
        LEFT_CHILD(TREE_PARENT(node))=right;
    }else{
        RIGHT_CHILD(TREE_PARENT(node))=right;
    }
    LEFT_CHILD(right)=node;
    TREE_PARENT(node)=right;
}

/*
 * branch_rotate_right -an operation affiliated to tree_fix, rotate node and children of node to the right
 */
void branch_rotate_right(void *node){
    void *left;

    left=LEFT_CHILD(node);
    LEFT_CHILD(node)=RIGHT_CHILD(left);
    if(RIGHT_CHILD(left)!=tree_null)
        TREE_PARENT(RIGHT_CHILD(left))=node;

    TREE_PARENT(left)=TREE_PARENT(node);
    if(node==LEFT_CHILD(TREE_PARENT(node))){
        LEFT_CHILD(TREE_PARENT(node))=left;
    }else{
        RIGHT_CHILD(TREE_PARENT(node))=left;
    }
    RIGHT_CHILD(left)=node;
    TREE_PARENT(node)=left;
}

/*
 * tree_fix - restore properties of Red-black tree after deleting
 */
void tree_fix(void *node){
    void *root, *sib;
   
    root=LEFT_CHILD(tree_root);
    while(!IS_RED(node) && node!=root){
        if(node==LEFT_CHILD(TREE_PARENT(node))){
            sib=RIGHT_CHILD(TREE_PARENT(node));
            if(IS_RED(sib)){
                IS_RED(sib)=0;
                IS_RED(TREE_PARENT(node))=1;
                branch_rotate_left(TREE_PARENT(node));
                sib=RIGHT_CHILD(TREE_PARENT(node));
            }
            if(!IS_RED(RIGHT_CHILD(sib)) && !IS_RED(LEFT_CHILD(sib))){
                IS_RED(sib)=1;
                node= TREE_PARENT(node);
            }else{
                if(!IS_RED(RIGHT_CHILD(sib))){
                    IS_RED(LEFT_CHILD(sib))=0;
                    IS_RED(sib)=1;
                    branch_rotate_right(sib);
                    sib=RIGHT_CHILD(TREE_PARENT(node));
                }
                IS_RED(sib)=IS_RED(TREE_PARENT(node));
                IS_RED(TREE_PARENT(node))=0;
                IS_RED(RIGHT_CHILD(sib))=0;
                branch_rotate_left(TREE_PARENT(node));
                node=root;
            }
        }else{
            sib=LEFT_CHILD(TREE_PARENT(node));
            if(IS_RED(sib)){
                IS_RED(sib)=0;
                IS_RED(TREE_PARENT(node))=1;
                branch_rotate_right(TREE_PARENT(node));
                sib=LEFT_CHILD(TREE_PARENT(node));
            }
            if(!IS_RED(RIGHT_CHILD(sib)) && !IS_RED(LEFT_CHILD(sib))){
                IS_RED(sib)=1;
                node=TREE_PARENT(node);
            }else{
                if(!IS_RED(LEFT_CHILD(sib))){
                    IS_RED(RIGHT_CHILD(sib))=0;
                    IS_RED(sib)=1;
                    branch_rotate_left(sib);
                    sib=LEFT_CHILD(TREE_PARENT(node));
                }
                IS_RED(sib)=IS_RED(TREE_PARENT(node));
                IS_RED(TREE_PARENT(node))=0;
                IS_RED(LEFT_CHILD(sib))=0;
                branch_rotate_right(TREE_PARENT(node));
                node=root;
            }
        }
        
    }
    IS_RED(node)=0;
}

/*
 * tree_delete - delete node from Red-black tree
 */
void tree_delete(void *node){
    void *target, *promoted_child;
    
    if(LEFT_CHILD(node)==tree_null || RIGHT_CHILD(node)==tree_null){
        target=node;
    }else{
        target=tree_successor(node);
    }

    if(LEFT_CHILD(target)==tree_null){
        promoted_child=RIGHT_CHILD(target);
    }else{
        promoted_child=LEFT_CHILD(target);
    }
    
    TREE_PARENT(promoted_child)=TREE_PARENT(target);
    if(TREE_PARENT(promoted_child)==tree_root){
        LEFT_CHILD(tree_root)=promoted_child;
    }else{
        if(LEFT_CHILD(TREE_PARENT(target))==target){
            LEFT_CHILD(TREE_PARENT(target))=promoted_child;
        }else{
            RIGHT_CHILD(TREE_PARENT(target))=promoted_child;
        }
    }

    if(target!=node){
        if(!IS_RED(target)){
            tree_fix(promoted_child);
        }
        LEFT_CHILD(target)=LEFT_CHILD(node);
        RIGHT_CHILD(target)=RIGHT_CHILD(node);
        TREE_PARENT(target)=TREE_PARENT(node);
        IS_RED(target)=IS_RED(node);
        TREE_PARENT(RIGHT_CHILD(node))=target;
        TREE_PARENT(LEFT_CHILD(node))=TREE_PARENT(RIGHT_CHILD(node));
        if(node==LEFT_CHILD(TREE_PARENT(node))){
            LEFT_CHILD(TREE_PARENT(node))=target;
        }else{
            RIGHT_CHILD(TREE_PARENT(node))=target;
        }
    }else{
        if(!IS_RED(target)){
            tree_fix(promoted_child);
        }
    }
}


/*
 * tree_insert - insert node into Red-black tree and color it so that all properties are preserved.
 */
void tree_insert(void *node){
    void *parent, *child, *sib;

    LEFT_CHILD(node)=RIGHT_CHILD(node)=tree_null;
    parent=tree_root;
    child=LEFT_CHILD(tree_root);
    while(child!=tree_null){
        parent=child;
        if(CUR_SIZE_MASKED(child)>CUR_SIZE_MASKED(node)){
            child=LEFT_CHILD(child);
        }else if(CUR_SIZE_MASKED(child)==CUR_SIZE_MASKED(node)){
            if(child>node){
                child=LEFT_CHILD(child);
            }else{
                child=RIGHT_CHILD(child);
            }
        }else{
            child=RIGHT_CHILD(child);
        }
    }
    TREE_PARENT(node)=parent;
    if(parent==tree_root || CUR_SIZE_MASKED(parent)>CUR_SIZE_MASKED(node)){
        LEFT_CHILD(parent)=node;
    }else if(CUR_SIZE_MASKED(parent)==CUR_SIZE_MASKED(node)){
        if(parent>node){
            LEFT_CHILD(parent)=node;
        }else{
            RIGHT_CHILD(parent)=node;
        }
    }else{
        RIGHT_CHILD(parent)=node;
    }

    IS_RED(node)=1;
    while(IS_RED(TREE_PARENT(node))){
        if(TREE_PARENT(node)==LEFT_CHILD(TREE_PARENT(TREE_PARENT(node)))){
            sib=RIGHT_CHILD(TREE_PARENT(TREE_PARENT(node)));
            if(IS_RED(sib)){
                IS_RED(TREE_PARENT(node))=0;
                IS_RED(sib)=0;
                IS_RED(TREE_PARENT(TREE_PARENT(node)))=1;
                node=TREE_PARENT(TREE_PARENT(node));
            }else{
                if(node==RIGHT_CHILD(TREE_PARENT(node))){
                    node=TREE_PARENT(node);
                    branch_rotate_left(node);
                }
                IS_RED(TREE_PARENT(node))=0;
                IS_RED(TREE_PARENT(TREE_PARENT(node)))=1;
                branch_rotate_right(TREE_PARENT(TREE_PARENT(node)));
            }
        }else{
            sib=LEFT_CHILD(TREE_PARENT(TREE_PARENT(node)));
            if(IS_RED(sib)){
                IS_RED(TREE_PARENT(node))=0;
                IS_RED(sib)=0;
                IS_RED(TREE_PARENT(TREE_PARENT(node)))=1;
                node=TREE_PARENT(TREE_PARENT(node));
            }else{
                if(node==LEFT_CHILD(TREE_PARENT(node))){
                    node=TREE_PARENT(node);
                    branch_rotate_right(node);
                }
                IS_RED(TREE_PARENT(node))=0;
                IS_RED(TREE_PARENT(TREE_PARENT(node)))=1;
                branch_rotate_left(TREE_PARENT(TREE_PARENT(node)));
            }
        }
    }
    IS_RED(LEFT_CHILD(tree_root))=0;
}





/*With these helper functions in hand, now we are ready to implement mm_malloc, mm_free and mm_realloc*/

/*
 * mm_malloc - Allocate a block
 *
 * If there exists a free block where the request fits, get the smallest one, segment it and allocate.
 * If there is no such block, increase brk.
 */
void *mm_malloc(size_t size)
{
    size_t block_size, next_block_size;
    void *free_block, *next_block;

    block_size=ALIGN(HEADER_SIZE+size);
    block_size=block_size < MIN_BLOCK_SIZE ? MIN_BLOCK_SIZE : block_size;
    free_block=tree_find(block_size);
    if(free_block==tree_null){/*no proper free block*/
	/*set free_block to the end of last block and increase heap*/
	free_block=mem_heap_hi()-WSIZE+1;
	if(GET_FREE(free_block)){/*read from the header if the last block is free*/
	    free_block=free_block-PREV_SIZE_MASKED(free_block);
	    if(IS_IN_TREE(free_block)){
		tree_delete(free_block);
	    }
	    /*Since this block is free, we only need to increase block-size minus size of this block*/
	    mem_sbrk(block_size-CUR_SIZE_MASKED(free_block));
	}else{/*the last block is not free*/
	    mem_sbrk(block_size);
	}
    }else{
        /*find a free block large enough*/
	tree_delete(free_block);
        next_block_size=CUR_SIZE_MASKED(free_block)-block_size;
	if(next_block_size>0){
            /*Divide the free block into two blocks, one for malloc and the other marked as free*/
	    next_block=NEXT_BLOCK(free_block, block_size);
            PREV_SIZE(NEXT_BLOCK(next_block,next_block_size))=next_block_size|1; /*set the header*/
	    CUR_SIZE(next_block)=next_block_size|1;
	    if(IS_IN_TREE(next_block)){
		tree_insert(next_block);
	    }
	}
    }
    CUR_SIZE(free_block)=block_size;
    PREV_SIZE(NEXT_BLOCK(free_block,block_size))=block_size;
    
    return USER_BLOCK(free_block);
}



/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size, new_size;
    void *prev, *cur, *next, *new_block;

    cur=ptr-HEADER_SIZE;

    if(CUR_FREE(cur)){
	printf("Double Free Error: try to free an already freed memory block(%p).\n",cur);
	return;
    }

    new_block=cur;
    new_size=CUR_SIZE_MASKED(cur);

    /*coalesce with the previous block if free*/
    if(PREV_FREE(cur)){
	size=PREV_SIZE_MASKED(cur);
	prev=PREV_BLOCK(cur,size);
	if(IS_IN_TREE(prev)){
	    tree_delete(prev);
	}
	new_block=prev;
	new_size+=size;
    }

    /*coalesce with the next block if exists and free*/
    size=CUR_SIZE_MASKED(cur);
    next=NEXT_BLOCK(cur,size);
    if(next+WSIZE<=mem_heap_hi() && CUR_FREE(next)){
	size=CUR_SIZE_MASKED(next);
	if(IS_IN_TREE(next)){
	    tree_delete(next);
	}
	new_size+=size;
    }

    /*Setting the new free block after coalesce*/
    CUR_SIZE(new_block)=new_size | 1;
    PREV_SIZE(NEXT_BLOCK(new_block,new_size))=new_size | 1;
    if(IS_IN_TREE(new_block)){
	tree_insert(new_block);
    }
}

/*
 * mm_realloc_last_resort - Implemented simply in terms of mm_malloc and mm_free. It's a helper function 
 *                          that will be invoked by mm_realloc if there are no other options.
 */

void *mm_realloc_last_resort(void *ptr, size_t size)
{
    void *oldptr=ptr;
    void *newptr;
    size_t copySize;

    newptr=mm_malloc(size);
    if(newptr==NULL)
        return NULL;
    copySize=CUR_SIZE_MASKED(oldptr-HEADER_SIZE)-HEADER_SIZE;
    if(size<copySize){
        copySize=size;
    }
    memcpy(newptr,oldptr,copySize);
    mm_free(oldptr);
    return newptr;
}

/*
 * mm_realloc - 
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr=ptr-HEADER_SIZE;
    void *old_next_block=NEXT_BLOCK(oldptr, CUR_SIZE_MASKED(oldptr));
    void *newptr;
    void *new_next_block;
    size_t remainder_size;
    size_t next_block_size;
    
    if(ptr==NULL){
        newptr=mm_malloc(size);
        return newptr;
    }

    if(size==0){
        mm_free(ptr);
        return NULL;
    }
    
    if(old_next_block==(mem_heap_hi()-WSIZE+1)){
        /*reaching the end of heap*/
        if(ALIGN(size+HEADER_SIZE)<=CUR_SIZE_MASKED(oldptr)){
            /*user requests to shrink the size*/
            size=ALIGN(size+HEADER_SIZE);
            if(size<CUR_SIZE_MASKED(oldptr)){
                /*segment the current block into two blocks*/
                remainder_size=CUR_SIZE_MASKED(oldptr)-size;
                CUR_SIZE(oldptr)=size;
                new_next_block=NEXT_BLOCK(oldptr,size);
                PREV_SIZE(new_next_block)=size;
                CUR_SIZE(new_next_block)=remainder_size | 1;
                PREV_SIZE(NEXT_BLOCK(new_next_block,remainder_size))=remainder_size | 1;
                if(IS_IN_TREE(new_next_block)){
                    tree_insert(new_next_block);
                }
                newptr=oldptr+HEADER_SIZE;
                return newptr;
            }else{
                newptr=oldptr+HEADER_SIZE;
                return newptr;
            }
        }else{
            /*user requests to increase the size, no other options*/
            return mm_realloc_last_resort(ptr, size);
        }
    }

    if(ALIGN(size+HEADER_SIZE)<=CUR_SIZE_MASKED(oldptr)){
        /*not reaching the end of heap while requesting to shrink the size*/
        size=ALIGN(size+HEADER_SIZE);
        if(size<CUR_SIZE_MASKED(oldptr)){
            /*segment the current block into two blocks*/
            remainder_size=CUR_SIZE_MASKED(oldptr)-size;
            CUR_SIZE(oldptr)=size;
            new_next_block=NEXT_BLOCK(oldptr,size);
            PREV_SIZE(new_next_block)=size;
            if(CUR_FREE(old_next_block)){
                /*coalesce with the next block if free(it is guaranteed to exist since it is not the end of heap)*/
                next_block_size=CUR_SIZE_MASKED(old_next_block);
                if(IS_IN_TREE(old_next_block)){
                    tree_delete(old_next_block);
                }
                remainder_size+=next_block_size;
                CUR_SIZE(new_next_block)=remainder_size | 1;
                PREV_SIZE(NEXT_BLOCK(new_next_block,remainder_size))=remainder_size | 1;
                if(IS_IN_TREE(new_next_block)){
                    tree_insert(new_next_block);
                }
                newptr=oldptr+HEADER_SIZE;
                return newptr;
            }else{
                /*no coalescence*/
                CUR_SIZE(new_next_block)=remainder_size | 1;
                PREV_SIZE(NEXT_BLOCK(new_next_block,remainder_size))=remainder_size | 1;
                if(IS_IN_TREE(new_next_block)){
                    tree_insert(new_next_block);
                }
                newptr=oldptr+HEADER_SIZE;
                return newptr;
            }
        }else{
            /*No need to segment*/
            newptr=oldptr+HEADER_SIZE;
            return newptr;
        }
    }else{
        /*User requests to increase the size*/
        size=ALIGN(size+HEADER_SIZE);
        if(CUR_FREE(old_next_block)){
            if((CUR_SIZE_MASKED(old_next_block)+CUR_SIZE_MASKED(oldptr)-size)>0){
                /*we will merge these two blocks into one since the sum of their size
                 * is strictly bigger than requested size*/
                remainder_size=CUR_SIZE_MASKED(old_next_block)+CUR_SIZE_MASKED(oldptr)-size;
                if(IS_IN_TREE(old_next_block)){
                    tree_delete(old_next_block);
                }
                new_next_block=NEXT_BLOCK(oldptr,size);
                CUR_SIZE(oldptr)=size;
                PREV_SIZE(new_next_block)=size;
                CUR_SIZE(new_next_block)=remainder_size | 1;
                PREV_SIZE(NEXT_BLOCK(new_next_block, remainder_size))=remainder_size | 1;
                if(IS_IN_TREE(new_next_block)){
                    tree_insert(new_next_block);
                }
                newptr=oldptr+HEADER_SIZE;
                return newptr;
            }else if((CUR_SIZE_MASKED(old_next_block)+CUR_SIZE_MASKED(oldptr)-size)==0){
                /*The sum of their size precisely equals the requested size*/
                if(IS_IN_TREE(old_next_block)){
                    tree_delete(old_next_block);
                }
                new_next_block=NEXT_BLOCK(oldptr,size);
                CUR_SIZE(ptr)=size;
                PREV_SIZE(new_next_block)=size;
                newptr=oldptr+HEADER_SIZE;
                return newptr;
            }else{
                /*The sum of their size is still not large enough, have to find new block*/
                return mm_realloc_last_resort(ptr,size);
            }
        }else{
            return mm_realloc_last_resort(ptr,size);
        }
    }
}



/*The following functions of the red-black tree are helper funtions for us to check the heap consistency*/

/*
 * tree_print_preorder_from - Starting from (void *)node, print the remaining nodes of the tree in a preorder fashion
 */
void tree_print_preorder_from(void *node){
    if(LEFT_CHILD(node)!=tree_null){
        tree_print_preorder_from(LEFT_CHILD(node));
    }
    printf("%p : %zu\n", node, CUR_SIZE_MASKED(node));
    if(RIGHT_CHILD(node)!=tree_null){
        tree_print_preorder_from(RIGHT_CHILD(node));
    }
}

/*
 * tree_print_preorder - print all nodes of the tree in a preorder fashion
 * (Note that LEFT_CHILD(tree_root) points to the TRUE tree_root by design.
 * Hence, tree_root is a special case)
 */
void tree_print_preorder(){
    printf("Printing all tree nodes for examination\n");
    if(LEFT_CHILD(tree_root)==tree_null){
        printf("empty\n");
    }else{
        tree_print_preorder_from(LEFT_CHILD(tree_root));
    }
}

/*
 * tree_check_preorder_from -check whether the tree has allocated blocks starint from (void *)node in a preorder fashion.
 */
int tree_check_preorder_from(void *node){
    if(LEFT_CHILD(node)!=tree_null){
        if(!tree_check_preorder_from(LEFT_CHILD(node))){
            return 0;
        }
    }
    if(!CUR_FREE(node)){
        printf("Error: %p is an allocated block in Red-black tree.\n", node);
        return 0;
    }
    if(RIGHT_CHILD(node)!=tree_null){
        if(!tree_check_preorder_from(RIGHT_CHILD(node))){
            return 0;
        }
    }
    return 1;
}

/*
 * tree_check_preorder -return 0 if there exists any allocated block in the tree, 1 otherwise.
 * (Similarly, LEFT_CHILD(tree_root) is the TRUE root of our tree)
 */
int tree_check_preorder(){
    if(LEFT_CHILD(tree_root)!=tree_null){
        return tree_check_preorder_from(LEFT_CHILD(tree_root));
    }
    return 1;
}


/*With all these helper functions, now we can implement mm_checkheap*/
void mm_checkheap(int verbose) 
{
    void *cur, *end;

    /*Is every block in the tree marked as free?*/
    if(tree_check_preorder()){
        printf("Pass: every block in the tree is marked as free\n");
    }

    if(verbose){
        tree_print_preorder();
    }

    /*Are there any contiguous free blocks that somehow escaped coalescing?*/
    cur=mem_heap_lo()+MIN_BLOCK_SIZE;
    end=mem_heap_hi()-WSIZE+1;
    while(cur<end){
        if(CUR_FREE(cur)){
            if(PREV_FREE(cur)){
                /*Contiguous free blocks detected*/
                printf("Error: %p,%p are contiguous free blocks\n", PREV_BLOCK(cur,CUR_SIZE_MASKED(cur)),cur);
                break;
            }
            /*Is every free block with size larger than MIN_BLOCK_SIZE actually tracked by the tree?*/
            if(IS_IN_TREE(cur) && !tree_find_exact(cur)){
                printf("Error: %p is a free block with size larger than MIN_BLOCK_SIZE, but it is not tracked by the tree\n", cur);
                break;
            }
        }
        if(verbose){
            printf("---[ %zu ]---", CUR_SIZE_MASKED(cur));
        }
        cur=NEXT_BLOCK(cur,CUR_SIZE_MASKED(cur));
    }
    printf("/n");
}











