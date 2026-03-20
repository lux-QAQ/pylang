Mark-sweep vs. copying collection and asymptotic complexity
[This page is based loosely on an impromptu presentation at IWMM '95. It needs work.]
A mark-sweep garbage collector traverses all reachable objects in the heap by following pointers beginning with the "roots", i.e. pointers stored in statically allocated or stack allocated program variables. All such reachable objects are marked. A sweep over the entire heap is the performed to restore unmarked objects to a free list, so they can be reallocated.

In contrast, a copying collector copies reachable objects to another region of memory as they are being traversed. Provided the traversal is done in breadth first order, there is a well-known and simple algorithm for performing this traversal without auxiliary storage or recursion. After such a traversal all surviving objects reside in a contiguous region of memory, and all pointers have been updated to point to the new object locations. The previously used region of memory can then be reused in its entirety. Allocation becomes trivial, since all free space is always contiguous.

Since a copying collector inherently moves objects, it is not suitable in environments (e.g. C with a standard compiler) in which most pointers cannot be identified with certainty. It would be impossible to correctly update such an ambiguous pointer. (There are adaptations such as Bartlett's which can tolerate a few ambiguous pointers.)

It has often been argued that copying collection is superior to mark-sweep for two reasons. First, it compacts memory, and hence avoids any fragmentation. Second, it's running time is proportional to the amount of live memory, not the size of the heap. We argue here that both are relevant only under special circumstances. Furthermore the second claim is meaningful only if garbage collection time is defined in a somewhat contrived manner.

We do not dispute the claims that copying garbage collection may often be easier to implement, or that allocation is faster, since free memory is contiguous. It is however our experience that in single-threaded environments with moderately long-lived objects garbage collection time generally dominates allocation time, even with noncontiguous free memory.

The following discussion compares pure copying collection to pure mark-sweep collection. Hybrids that copy only young objects are in fact quite desirable in some circumstanes, in spite of the arguments that follow. We also do not address ``fake copying'' of large objects that involves page remapping. Memory remapping in fact makes sense for both copying and mark-sweep collection, with the result that both should behave essentially identically for large objects. (In the case of a mark-sweep collector, fragmentation can be significantly reduced by unmapping unused memory chunks if the necessary facilites are available.)

Memory compaction -
Recent measurements by Paul Wilson
 as well as by Ben Zorn's group at the University of Colorado suggest that a good noncompacting allocator is very unlikely to result in more than half of memory being lost to fragmentation. In contrast, a copying collector can never use more than half of available memory, since it needs to reserve the other half for copying reachable objects during a collection. This implies that in a virtual memory environment, the copying collector is likely to start paging first. A copying collector exhibits better space performance in the worst case. However, under most realistic circumstances, a noncompacting mark-sweep collector is likely to use less space. (There are not enough reliable measurements available to make this claim with absolute certainty and generality. Very long running programs have rarely been measured. The relative performance of the two GC algorithms when both are paging is uncertain.)
Another popular argument in favor of copying collection is that as a result of compaction, the application's working set size is reduced. The collector will touch more memory during garbage collection. But such access is, as Henry Baker has correctly pointed out, mostly sequential. And subsequently the client program will see its data compacted into a smaller section of the address space.

This is an appealing theoretical argument. On the other hand, it is unconvincing on a typical modern workstation. Consider a situation in which it is known that only N bytes will be live at any point. Assume the heap size is 3N bytes. A copying collection is triggered every N bytes of allocation. The machine has 2N bytes available physical memory, and none of the newly allocated memory survives. Thus the copying collection consists of an N byte sequential copy.

Assume that at the beginning of a copy, the target space is not resident. (This is no worse than any other paging strategy, especially since it could have been discarded without writing to disk.) Thus each page copied requires a new page to be allocated. Since the space containing old objects is still potentially live until the end of the collection, this requires one page of the old space to be written to disk. Thus, under optimal conditions, each byte of allocation requires a byte written to the disk. (In reality, things won't be this smooth, and there will be contention between old and new space. There also likely to be more random accesses involved, casuing significant seek delays. But we'll give the give the copying collector the benefit of the doubt, for now.)

To put this into perspective, our conservative collector, running with default parameters (which require more GC work and less physical memory than the above scenario) allocates, clears, and reclaims memory at about 10MB/sec on a 133MHz R4600 Indy (not a terribly fast processor by modern standards). This is considerably faster than disk transfer rates for conventional disk drives, even if purely sequential transfers were involved. In fact, memory accesses go through the paging system, and sequential virtual memory may or may not be mapped to sequential disk locations, there is OS kernel overhead, etc. The same machine can clear pages which had not been previously touched (and thus require writing other pages to disk) at about 3MB/sec. This was measured with an sbrk call followed by a memset for a region that significantly exceeded the physical memory size of the machine.) This latter number is not much different on newer machines which can sustain higher mark/sweep allocation rates.

Thus copying collection is bound to lose for applications that barely page, just by virtue of writing out a major part of the address space once per GC cycle. For applications that page heavily, noncompacting collection has the added advantage that pointerfree objects do not need to be examined by the collector (see below). There is no convincing evidence that compaction gives a compensating benefit, or a significant benefit at all for most applications, especially since the object ordering obtained by simple copying collectors (breadth-first ordering) is unlikely to correspond to the order in which the client touches memory. A mark-sweep collector generally allocates consecutive objects at nearby addresses, which is likely to be a more useful property.

Asymptotic complexity -
A complete garbage collection/allocation cycle consists of the following phases:
0) Initialize for trace, e.g. clear mark bits

I) Tracing (marking or copying)

II) Sweeping

III) Allocate reclaimed storage

Phase 0 is negligible for all algorithms in practice, and could be done in constant time with suitable clever data structures. Hence we will ignore it.

Phase I takes time proportional at most to the amount of reachable data for either copying or mark-sweep collectors. For a mark-sweep collector, the running time of this phase is really only proportional to the number of pointers in the heap.

Phase II is unnecessary for a copying collector. For a traditional mark-sweep collector, it takes time proportional to the size of the heap. This is the basis of the claim that copying collection exhibits superior asymptotic performance. However, there is no inherent reason for a nonmoving marking collector to require a sweep phase. Indeed, it is possible to simply mark reachable objects and have the allocator search for unmarked objects. The search will terminate quickly in precisely those cases in which a copying collector is claimed superior, namely when most of the heap is empty. Moreover, it is possible to represent mark bits as doubly linked lists, in which case no search is necessary. (This is the essence of Baker's treadmill garbage collector.)

Even if there is an explicit sweep phase, it rarely dominates execution time. The sweep phase only has to examine mark bits, not the actual contents of objects. Well-designed collectors concerned with paging and cache locality are likely to keep mark bits in a separate region of memory, sometimes making it possible to examine several mark bits at once. It may be necessary to update a pointer within the object to add it to a free list. But a well-designed collector will perform this operation incrementally, interleaved with allocation. Thus any page faults or cache misses incurred in the process would have otherwise been incurred by the immediately following allocation of the object. The net cost of a free list insertion should thus consist of two pointer writes and one read, all of which are effectively cache hits. This is much less than the cost of either traversing an object for marking or copying an object.

Phase III essentially takes time proportional to the unmarked (or uncopied) section of the heap. (This requires the assumption that either the average object size is bounded or heap objects are initialized. Both assumptions are reasonable.)

Thus the cost of the entire allocation cycle is proportional to the heap size, independent of the collection algorithm. The supposed difference in complexity affects at most one ill-defined section of the program that is not normally dominant. It can never affect the asymptotic complexity of an entire program unless that program allocates large amounts of memory that are never initialized.

Furthermore, a well designed collector typically keeps the size of the heap at a small multiple of the reachable data size. It must be kept at a multiple, in order to keep the fraction of time spent in the garbage collector fixed. Making the heap larger than about twice the size of reachable data is usually undesirable on machines running more than one process, since the resulting space requirements are often perceived as unreasonable. Our collector often keeps the ratio at about 1.5. Thus the real difference between heap size and reachable data size is a small constant. It is typically less than the ratio of total reachable data size to total number of reachable pointers. And the fact that a mark-sweep collector only needs to examine pointers is usually ignored.

The bottom line
The fact that copying collectors offer much faster allocation than the allocation+sweep time of a mark-sweep collector means that they are still attractive for the youngest generation(s) in generational collectors, where the copy time can be kept very low. But the crucial factor here is the small constant factor in the allocation time, and the fact that young objects can be segregated, thus making it easier to identify old unmodified objects. This does not reflect any difference in asymptotic complexity.
Copying collectors can also offer better worst-case space bounds than nonmoving collectors, particularly if real-time constraints are present. Thus they may be appropriate for certain embedded real-time applications.

There is growing evidence that copying collectors are a poor choice for old objects in a typical desktop application. They often result in unnecessarily large memory footprints, and paging where none is really necessary. The net effect can be disastrous performance.


Hans-J. Boehm
(boehm@acm.org)
This is a revised version of a page written while the author was at Xerox PARC. The original is here
.
GC for C/C++ home page

