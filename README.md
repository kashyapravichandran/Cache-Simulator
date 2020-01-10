# Cache Simulator

The program tries to simulate a memory heirarchy consisting of a L1 and a L2 cache n-way set associative cache. The program takes the block size, the cache size, set associativity of the cache. The simulator assumes that the block size of both the caches are equal to make things easier. It follows a WBWA policy. 

The output of the simulator is the tag store contents and metrics like read misses, write misses, write backs and miss rates for the both the caches. 

Apart from this, the program also simulates the working of the L1 n-way set associative cache and a L2 direct mapped decoupled sectored cache. With the L2 cache being decoupled, we reduce the tag store size and eliminate the one to one mapping of tags to the data blocks. In addition to the parameters specified earlier, the program also takes in the  and the number of tags and the number of blocks in a sector that the cache should have. 

## Usage

In a linus machine run make and the following command: 

            ./sim_cache <block size> <L1 Size> <L1 Associativity> <L2 Size> <L2 Associativity> <Number of Data Block in a sector (L2)> <Number of tags> <Trace File>
            
            when you are trying to simulate a memory heirarchy with just L1 cache, have the size of L2 cache as zero. 
            when you are trying to simulate a heirarchy with L1 and L2 n-way set associative cahce, pass 1 as arguments to both the number of data blocks in a sector and number of tags.
           
         
In a windows machine, compile and create and execution file and pass arguments to the program as mentioned above. 

More details about decoupled sectore cache can be found in the project spec document and on the paper and that is linked in the document.
