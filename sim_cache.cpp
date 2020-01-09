/* Project 1B. Multilevel level cache simulation
L2 can be either a N way set associative cache or a direct mapped sectored cache
ECE 563: Microarchitecture
Kashyap Ravichandran

*/

// DIrty bit : Set - dirty, reset - Not dirty
// Valid: Set - Dirty, resert - Not Dirty

// need to implement writebacks

#include<iostream>
#include<math.h>
#include<fstream>
#include<string>
#include<stdlib.h>
#include<iomanip> 
using namespace std;	

struct L2_cache
{
	int dirty,valid,tag;
};


int miss=0;
class cache
{
	int BLOCKSIZE;
	int offset;
	int tag_from_address;
	int index_from_address;
	int L1_SIZE;
	int L1_ASSOC;
	int L2_SIZE;
	int L2_ASSOC;
	int L2_DATA_BLOCK;
	int L2_ADDR_TAGS;
	int **tag; 
	int **lru;
	int **selection_bits;
	int **valid_flag;
	int **valid_tag;
	L2_cache **sec;
	int index;
	int index_bits;
	int block_bits;
	cache *nextlevel;
	int **dirty_flag;// to identify if a dirty block is evicted
	int c0_bits;
	int c1_bits;
	int c2_bits;
	
public: 
// initializing the attributes of class cache
	int reads, writes, rmiss, wmiss, write_back,sector_miss,cache_block_miss;
	cache(int bs,int l1_size, int l1_assoc, cache *p=NULL )
	{
		reads=0;
		writes=0;
		rmiss=0;
		wmiss=0;
		write_back=0;
		BLOCKSIZE=bs;
		L1_SIZE=l1_size;
		L2_ADDR_TAGS=1;
		L2_DATA_BLOCK=1;
		L1_ASSOC=l1_assoc;
		nextlevel=p; // NULL cause there is no next level
		index=L1_SIZE/(BLOCKSIZE*L1_ASSOC);
		
		tag = new int* [index];
		for(int i=0;i<index;i++)
			tag[i]=new int[L1_ASSOC];
		
		for(int i=0;i<index;i++)
		for(int j=0;j<L1_ASSOC;j++)
			tag[i][j]=0;
		
		valid_flag = new int* [index];
		for(int i=0;i<index;i++)
			valid_flag[i]=new int[L1_ASSOC];
		
		lru = new int* [index];
		for(int i=0;i<index;i++)
			lru[i]=new int[L1_ASSOC];	
		
		dirty_flag = new int* [index];
		for(int i=0;i<index;i++)
			dirty_flag[i]=new int[L1_ASSOC];
		
		index_bits=log2(index);
		block_bits=log2(BLOCKSIZE);
		
		for(int i =0;i<index;i++)
		for(int j=0;j<L1_ASSOC;j++)
		dirty_flag[i][j]=0;
		
		for(int i =0;i<index;i++)
		for(int j=0;j<L1_ASSOC;j++)
		valid_flag[i][j]=0;	
		
		for(int i =0;i<index;i++)
		for(int j=0;j<L1_ASSOC;j++)
		lru[i][j]=L1_ASSOC;
		
		//cout<<"index "<<index;
		//cout<<"\n index bits"<<index_bits;
		//cout<<"\noffset bits"<<block_bits;
	
	}
	
	cache(int bs,int l1_size,int assoc,int addr, int data)
	{
		reads=0;
		writes=0;
		rmiss=0;
		wmiss=0;
		write_back=0;
		sector_miss=0;
		cache_block_miss=0; 
		 
		BLOCKSIZE=bs;
		L1_SIZE=l1_size;
		L2_DATA_BLOCK=data;
		L2_ADDR_TAGS=addr;
		L1_ASSOC=assoc;
		nextlevel=NULL; // NULL cause there is no next level
		
		index=L1_SIZE/(BLOCKSIZE*L2_DATA_BLOCK);
		block_bits=log2(BLOCKSIZE);
		c0_bits=log2(L2_DATA_BLOCK);
		c1_bits=log2(index);
		c2_bits=log2(L2_ADDR_TAGS);
		
		//cout<<c0_bits<<"  "<<c1_bits<<"   "<<c2_bits<<block_bits;
		tag = new int* [index];
		for(int i=0;i<index;i++)
			tag[i]=new int[L2_ADDR_TAGS];
		
		for(int i=0;i<index;i++)
			for(int j=0;j<L2_ADDR_TAGS;j++)
				tag[i][j]=0;
		
		valid_tag = new int* [index];
		for(int i=0;i<index;i++)
			valid_tag[i]=new int[L2_ADDR_TAGS];
		
		for(int i=0;i<index;i++)
			for(int j=0;j<L2_ADDR_TAGS;j++)
				valid_tag[i][j]=0;

		
		valid_flag = new int* [index];
		for(int i=0;i<index;i++)
			valid_flag[i]=new int[L2_DATA_BLOCK];
		
		for(int i =0;i<index;i++)
			for(int j=0;j<L2_DATA_BLOCK;j++)
				valid_flag[i][j]=0;	
				
		
		dirty_flag = new int* [index];
		for(int i=0;i<index;i++)
			dirty_flag[i]=new int[L2_DATA_BLOCK];
						
		for(int i =0;i<index;i++)
			for(int j=0;j<L2_DATA_BLOCK;j++)
				dirty_flag[i][j]=0;
		
		selection_bits = new int* [index];
		for(int i=0;i<index;i++)
			selection_bits[i]= new int [L2_DATA_BLOCK];
			
		for(int i=0;i<index;i++)
			for(int j=0;j<L2_DATA_BLOCK;j++)
				selection_bits[i][j]=0;
		
		sec=new L2_cache* [index];
		for(int i=0;i<index;i++)
		{
			sec[i]= new L2_cache [L2_DATA_BLOCK];
			for(int j=0;j<L2_DATA_BLOCK;j++)
			{
				sec[i][j].dirty=0;
				sec[i][j].valid=0;
				sec[i][j].tag=0;
			}

		}
				
						
	}
	
	bool readsector(int address)
	{
		int ra_tag,ra_selection,ra_sector,ra_index;
		split(address, ra_tag,ra_index,ra_sector,ra_selection);
		
		if(valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]==ra_selection&&tag[ra_index][ra_selection]==ra_tag)
		{
			return true;
		}
		
		//readsector1(address);
		sector(ra_index);
		
		if(!valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]!=ra_selection&&tag[ra_index][ra_selection]!=ra_tag)
		{
			
			for(int i=0;i<L2_DATA_BLOCK;i++)
			{
				if(valid_flag[ra_index][i]&&selection_bits[ra_index][i]==ra_selection&&ra_sector!=i)
				{
					selection_bits[ra_index][i]=0;
					valid_flag[ra_index][i]=0;
					if(dirty_flag[ra_index][i])	
						write_back++;
					dirty_flag[ra_index][i]=0;
				}
			}
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=0;
			return false;
		}

		else if(!valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]!=ra_selection&&tag[ra_index][ra_selection]==ra_tag)
		{	
			
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=0;
			return false;
		}
	
		else if(!valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]==ra_selection&&tag[ra_index][ra_selection]!=ra_tag)
		{
			
			for(int i=0;i<L2_DATA_BLOCK;i++)
			{
				if(valid_flag[ra_index][i]&&selection_bits[ra_index][i]==ra_selection&&ra_sector!=i)
				{
					selection_bits[ra_index][i]=0;
					valid_flag[ra_index][i]=0;
					if(dirty_flag[ra_index][i])
						write_back++;
					dirty_flag[ra_index][i]=0;
				}
			}
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=0;
			return false;
		}	

		else if(!valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]==ra_selection&&tag[ra_index][ra_selection]==ra_tag)
		{
			
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=0;
			return false;
		}

		else if(valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]!=ra_selection&&tag[ra_index][ra_selection]!=ra_tag)
		{
			
			for(int i=0;i<L2_DATA_BLOCK;i++)
			{
				if(valid_flag[ra_index][i]&&selection_bits[ra_index][i]==ra_selection&&ra_sector!=i)
				{	
					selection_bits[ra_index][i]=0;
					valid_flag[ra_index][i]=0;
					if(dirty_flag[ra_index][i])
						write_back++;
					dirty_flag[ra_index][i]=0;
				}
			}	
			if(dirty_flag[ra_index][ra_sector])
				write_back++;
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=0;
			return false;
		}	

		else if(valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]!=ra_selection&&tag[ra_index][ra_selection]==ra_tag)
		{	
			
			if(dirty_flag[ra_index][ra_sector])
				write_back++;	
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=0;
			return false;
		}

		else if(valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]==ra_selection&&tag[ra_index][ra_selection]!=ra_tag)
		{
			
			for(int i=0;i<L2_DATA_BLOCK;i++)
			{
				if(valid_flag[ra_index][i]&&selection_bits[ra_index][i]==ra_selection&&ra_sector!=i)
				{
					selection_bits[ra_index][i]=0;
					valid_flag[ra_index][i]=0;	
					if(dirty_flag[ra_index][i])
						write_back++;
					dirty_flag[ra_index][i]=0;
				}
			}
			if(dirty_flag[ra_index][ra_sector])
				write_back++;
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=0;
			return false;
				 }
	
	return false;
			
	}
	
	bool writesector(int address)
	{
		int ra_tag,ra_index,ra_sector,ra_selection;
		split(address,ra_tag,ra_index,ra_sector,ra_selection);
		//writesector1(address);
			//it
		if(valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]==ra_selection&&tag[ra_index][ra_selection]==ra_tag)
		{
			dirty_flag[ra_index][ra_sector]=1;
			return true;
		}
		
		sector(ra_index);
		if(!valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]!=ra_selection&&tag[ra_index][ra_selection]!=ra_tag)
		{
			
			for(int i=0;i<L2_DATA_BLOCK;i++)
			{
				if(valid_flag[ra_index][i]&&selection_bits[ra_index][i]==ra_selection&&ra_sector!=i)
				{
					selection_bits[ra_index][i]=0;
					valid_flag[ra_index][i]=0;
					if(dirty_flag[ra_index][i])
						write_back++;
					dirty_flag[ra_index][i]=0;
				}
			}
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=1;
			return false;
		}
	
		else if(!valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]!=ra_selection&&tag[ra_index][ra_selection]==ra_tag)
		{
			
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=1; 
			return false;
		}

		else if(!valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]==ra_selection&&tag[ra_index][ra_selection]!=ra_tag)
		{
			
			for(int i=0;i<L2_DATA_BLOCK;i++)
			{
				if(valid_flag[ra_index][i]&&selection_bits[ra_index][i]==ra_selection&&ra_sector!=i)
				{
					selection_bits[ra_index][i]=0;
					valid_flag[ra_index][i]=0;
					if(dirty_flag[ra_index][i])
						write_back++;
					dirty_flag[ra_index][i]=0;
				}
			}
			tag[ra_index][ra_selection]=ra_tag;	
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=1;
			return false;
		}
	
		else if(!valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]==ra_selection&&tag[ra_index][ra_selection]==ra_tag)	
		{
			
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=1;
			return false;
		}

		else if(valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]!=ra_selection&&tag[ra_index][ra_selection]!=ra_tag)
		{
			
			for(int i=0;i<L2_DATA_BLOCK;i++)
			{
				if(valid_flag[ra_index][i]&&selection_bits[ra_index][i]==ra_selection&&ra_sector!=i)
				{
					selection_bits[ra_index][i]=0;
					valid_flag[ra_index][i]=0;
					if(dirty_flag[ra_index][i])
						write_back++;
					dirty_flag[ra_index][i]=0;
				}
			}
			if(dirty_flag[ra_index][ra_sector])
				write_back++;
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=1;
			return false;
		}

		else if(valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]!=ra_selection&&tag[ra_index][ra_selection]==ra_tag)
		{
			
			if(dirty_flag[ra_index][ra_sector])
				write_back++;	
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=1;
			return false;
		}
	
		else if(valid_flag[ra_index][ra_sector]&&selection_bits[ra_index][ra_sector]==ra_selection&&tag[ra_index][ra_selection]!=ra_tag)
		{
			
			for(int i=0;i<L2_DATA_BLOCK;i++)
			{
				if(valid_flag[ra_index][i]&&selection_bits[ra_index][i]==ra_selection&&ra_sector!=i)
				{
					selection_bits[ra_index][i]=0;
					valid_flag[ra_index][i]=0;
					if(dirty_flag[ra_index][i])
						write_back++;
					dirty_flag[ra_index][i]=0;
				}
			}
			if(dirty_flag[ra_index][ra_sector])
				write_back++;
			tag[ra_index][ra_selection]=ra_tag;
			selection_bits[ra_index][ra_sector]=ra_selection;
			valid_flag[ra_index][ra_sector]=1;
			dirty_flag[ra_index][ra_sector]=1;
			return false;
			
			}

	return false;
			
	}	
	
	bool readFromAddress(int address)
	{
		//cout<<"HEre";
		int ra_tag,ra_index;
		split(address,ra_tag,ra_index);
		for(int i=0;i<L1_ASSOC;i++)
		{
			if(tag[ra_index][i]==ra_tag&&valid_flag[ra_index][i]) // hit
			{
				//update LRU
				update_lru(ra_index,i);
				
				return true;
			}
		}
		
		
		for(int i=0;i<L1_ASSOC;i++)
		{
			if(!valid_flag[ra_index][i])
			{
				
				tag[ra_index][i]=ra_tag; // invalid miss
				//update LRU Counter
				lru[ra_index][i]=0;
				for(int j=0;j<L1_ASSOC;j++)
				{
					if(j!=i&&valid_flag[ra_index][j])
						lru[ra_index][j]++;
					
				}
				//add one to every lru?
				dirty_flag[ra_index][i]=0;
				valid_flag[ra_index][i]=1;
				
					if(nextlevel)
		{
			if(nextlevel->L2_ADDR_TAGS==1)
			{	
				nextlevel->reads++;
				bool test=nextlevel->readFromAddress(address);
				if(!test)
					nextlevel->rmiss++;
			}
			if(nextlevel&&nextlevel->L2_ADDR_TAGS!=1)
			{	
				
				nextlevel->reads++;
				bool test=nextlevel->readsector(address);
				if(!test)
					nextlevel->rmiss++;
			}
		}
				
				return false;
				
			}
					
		}			
		int lru_hor_index=0;
		for(int i =0;i<L1_ASSOC;i++)
		{
			// Find Least recently used memeory
			if(lru[ra_index][i]==(L1_ASSOC-1)&&valid_flag[ra_index][i])
			{
				lru_hor_index=i;
				break;
			}
		}
		if(dirty_flag[ra_index][lru_hor_index])
		{
			
			write_back++;
			if(nextlevel)
			{
				
				if(nextlevel->L2_ADDR_TAGS==1)
				{
					int temp;
					reconstruct(tag[ra_index][lru_hor_index],ra_index,temp);
					nextlevel->writes++;
					bool test=nextlevel->writeToAddress(temp);
					if(!test)
						nextlevel->wmiss++;
				
				}
				if(nextlevel&&nextlevel->L2_ADDR_TAGS!=1)
				{
					
					int temp;
					reconstruct(tag[ra_index][lru_hor_index],ra_index,temp);
					nextlevel->writes++;
					bool test=nextlevel->writesector(temp);
					if(!test)
						nextlevel->wmiss++;
				
				}
			}
		}

		tag[ra_index][lru_hor_index]=ra_tag;
		lru[ra_index][lru_hor_index]=0;
		for(int j=0;j<L1_ASSOC;j++)
		{
			if(j!=lru_hor_index&&valid_flag[ra_index][j])
				lru[ra_index][j]++;
					
		}
		
		valid_flag[ra_index][lru_hor_index]=1;	
		if(nextlevel)
		{
			if(nextlevel->L2_ADDR_TAGS==1)
			{	
				nextlevel->reads++;
				bool test=nextlevel->readFromAddress(address);
				if(!test)
					nextlevel->rmiss++;
			}
			if(nextlevel&&nextlevel->L2_ADDR_TAGS!=1)
			{	
				
				nextlevel->reads++;
				bool test=nextlevel->readsector(address);
				if(!test)
					nextlevel->rmiss++;
			}
		}

		dirty_flag[ra_index][lru_hor_index]=0;
		return false;
		
	}
	
	bool writeToAddress(int address )
	{
		//cout<<"Hey";
		int ra_tag,ra_index;
		split(address,ra_tag,ra_index);
		//cout<<ra_tag<<ra_index;
		for(int i=0;i<L1_ASSOC;i++)
		{
			if(valid_flag[ra_index][i])
				if(tag[ra_index][i]==ra_tag) // hit
				{
					update_lru(ra_index,i);
					dirty_flag[ra_index][i]=1;
					
					return true;

				}
		}
		
		for(int i=0;i<L1_ASSOC;i++)
		{
			//cout<<"Hey";
			if(!valid_flag[ra_index][i])
			{
				tag[ra_index][i]=ra_tag; // invalid miss
				//update LRU Counter
				dirty_flag[ra_index][i]=1;
				update_lru(ra_index,i);
				valid_flag[ra_index][i]=1;
				//cout<<"Heiyo";
				//cout<<nextlevel;
				if(nextlevel)
				{	
					//cout<<"Hello";
					if(nextlevel->L2_ADDR_TAGS==1)
					{
						//cout<<"I am here right";
						nextlevel->reads++;
						bool test=nextlevel->readFromAddress(address);
						if(!test)
							nextlevel->rmiss++;
					}
					if(nextlevel&&nextlevel->L2_ADDR_TAGS!=1)
					{	
						//cout<<"HEy";
						
						nextlevel->reads++;
						bool test=nextlevel->readsector(address);
						if(!test)
							nextlevel->rmiss++;
					}
				}
			return false;
				
			}
					
		}			
		int lru_hor_index=0;
		for(int i =0;i<L1_ASSOC;i++)
		{
			// Find Least recently used memeory
			if(lru[ra_index][i]==(L1_ASSOC-1)&&valid_flag[ra_index][i])
			{
				lru_hor_index=i;
			}
		}
		// Replace and update LRU
		if(dirty_flag[ra_index][lru_hor_index])
		{
			
			write_back++;
			if(nextlevel)
			{
				
				if(nextlevel->L2_ADDR_TAGS==1)
				{
					int temp;
					reconstruct(tag[ra_index][lru_hor_index],ra_index,temp);
					nextlevel->writes++;
					bool test=nextlevel->writeToAddress(temp);
					if(!test)
						nextlevel->wmiss++;
				
				}
				if(nextlevel&&nextlevel->L2_ADDR_TAGS!=1)
				{
					int temp;
					
					reconstruct(tag[ra_index][lru_hor_index],ra_index,temp);
					nextlevel->writes++;
					bool test=nextlevel->writesector(temp);
					if(!test)
						nextlevel->wmiss++;
				
				}
			}
		}
		
		tag[ra_index][lru_hor_index]=ra_tag;
		valid_flag[ra_index][lru_hor_index]=1;
		
		if(nextlevel)
		{
			if(nextlevel->L2_ADDR_TAGS==1)
			{	
				nextlevel->reads++;
				bool test=nextlevel->readFromAddress(address);
				if(!test)
					nextlevel->rmiss++;
			}
			if(nextlevel&&nextlevel->L2_ADDR_TAGS!=1)
			{	
				
				nextlevel->reads++;
				bool test=nextlevel->readsector(address);
				if(!test)
					nextlevel->rmiss++;
			}
		}
		
		update_lru(ra_index,lru_hor_index);
		dirty_flag[ra_index][lru_hor_index]=1;

			
		return false;
	}

	void split(int address, int &ra_tag, int &ra_index, int &ra_sector, int &ra_selection)
	{
		int temp=address;
		
		int temp1=pow(2,block_bits)-1; 
		offset=temp&temp1;
		temp=temp>>block_bits; 
		
		temp1=pow(2,c0_bits)-1;
		ra_sector=temp&temp1; 
		temp=temp>>c0_bits;
		
		temp1=pow(2,c1_bits)-1;
		ra_index=temp&temp1;
		temp=temp>>(c1_bits); 
		
		temp1=pow(2,c2_bits)-1;
		ra_selection=temp&temp1;
		temp=temp>>c2_bits; 
		
		ra_tag=temp; 
		//
		//cout<<ra_tag<<" "<<ra_selection<<" "<<ra_index<<" "<<ra_selection<<"  "<<offset<<" \n ";	
	
	}
	
	void split(int address, int &tag_address,int &index_address )
	{
		int temp2=0;
		temp2=pow(2,block_bits)-1;
		offset=address&temp2;
		int temp= address>>(block_bits);
		int temp1=1;
		temp1=pow(2,index_bits)-1;
		index_address=temp&temp1;
		tag_address=address>>(block_bits+index_bits);
		
	}
	
	void reconstruct(int ra_tag, int ra_index, int &address)
	{
		address=0;
		address=ra_tag<<(block_bits+index_bits)|ra_index<<block_bits|0;
		
	}

	void update_lru(int index,int lru_index)
	{
		// Memeber function to update the LRU counter
		int temp=lru[index][lru_index];
		lru[index][lru_index]=0;
		for(int i=0;i<L1_ASSOC;i++)
		{
			if(lru[index][i]<temp&&valid_flag[index][i]&&lru_index!=i)
				lru[index][i]++;
		}
	}
	
	void output1(char a[])
	{

		for(int i=0;i<index;i++)
		{
			for(int j=0;j<L1_ASSOC-1;j++)
			for(int k=0;k<L1_ASSOC-1-j;k++)
				if(lru[i][k]>lru[i][k+1])
				{
					int t= tag[i][k];
					tag[i][k]=tag[i][k+1];
					tag[i][k+1]=t;
					t=dirty_flag[i][k];
					dirty_flag[i][k]=dirty_flag[i][k+1];
					dirty_flag[i][k+1]=t;
					t=lru[i][k];
					lru[i][k]=lru[i][k+1];
					lru[i][k+1]=t;
					
					
				}			
		}
		
		for(int i =0;i<index;i++)
		{
			cout<<"set "<<i<<":";
			for(int j=0;j<L1_ASSOC;j++)
			{
				cout<<"  "<<std::hex<<tag[i][j]<<std::dec<<" ";
				if(dirty_flag[i][j])
				cout<<"D "<<" ||";
				else 
				cout<<"N "<<" ||";
			}
			cout<<"\n";
		}
	

	}
	
	void output2()
	{
		for(int i=0;i<index;i++)
		{
			cout<<"set "<<i<<":";
			for(int j=0;j<L2_ADDR_TAGS;j++)
			{
				cout<<"  "<<hex<<tag[i][j]<<dec<<"  ";
			}
			cout<<"  ||\n";
		}
		
	}
	
	void output3()
	{
		for(int i=0;i<index;i++)
		{
			cout<<"set "<<i<<": \t";
			for(int j=0;j<L2_DATA_BLOCK;j++)
			{
				cout<<" "<<selection_bits[i][j]<<",";
				if(valid_flag[i][j])
					cout<<"V,";
				else 
					cout<<"I,";
				if(dirty_flag[i][j])
					cout<<"D";
				else
					cout<<"N";
					cout<<"\t";
			}
			cout<<"   ||\n";
		}
	}
	void sector(int ra_index)
	{
		int count=0;		
		for(int i=0;i<L2_DATA_BLOCK;i++)
		{
			if(valid_flag[ra_index][i])
				count++;
			
		}
		if(!count)
			sector_miss++;
		else 
			cache_block_miss++;
	}
};

int main(int argc, char* argv[]) //  pararmenter in main

{
	//int no_reads =0,no_writes =0, read_misses =0, write_misses =0;
	if(!argc)
	{
		cout<<" Invalid call, change it!";
		return 0;
	}
	int arg[7]={0};
	int temp;
	for(int i=1;i<=7;i++)
		{
			for(int j=0;argv[i][j];j++)
			{
				arg[i-1]*=10;
				temp=argv[i][j]-48;
				arg[i-1]+=temp;
			}
		}
	char temp1[50];
	ifstream fin;
	fin.open(argv[8]);
	if(fin.is_open())
	{
	//cout<<"FILEEE\n\n\n";
		if(arg[5]==1 && arg[6]==1&&arg[3])
		{
			//cout<<"WTF";
			int address=0;
			//cache *p=NULL;
			cache L2(arg[0],arg[3],arg[4]);
			cache L1(arg[0],arg[1],arg[2],&L2);
			//cout<<"WTF again";
			bool test;
			while(!fin.eof())
			{
				fin.getline(temp1,11,'\n');
				address=0;
				char *temp2=(temp1+2);	
				//cout<<temp1;
				address=strtol(temp2,NULL,16);	
				if(temp1[0]=='r'||temp1[0]=='R')
				{
					L1.reads++;
					//cout<<"Here now";
					test=L1.readFromAddress(address);
					if(!test)
						L1.rmiss++;						
				}
				else if (temp1[0]=='w'||temp1[0]=='W')
				{
					L1.writes++;
					test=L1.writeToAddress(address);
					if(!test)	
						L1.wmiss++;
				}	
		
	
			}	
	
			cout<<"===== Simulator configuration =====\n";
			cout<<"Blocksize:						  "<<arg[0]<<"\n";
			cout<<"L1_Size:					          "<<arg[1]<<"\n";
			cout<<"L1_ASSOC:						  "<<arg[2]<<"\n";	
			cout<<"L2_SIZE:						  "<<arg[3]<<"\n";
			cout<<"L2_ASSOC:						  "<<arg[4]<<"\n";	
			cout<<"L2_DATA_BLOCKS:						  "<<arg[5]<<"\n";
			cout<<"L2_ADDRESS_TAGS:					  "<<arg[6]<<"\n";
			cout<<"trace_file:                     \t \t\t  "<<argv[8]<<"\n\n";
			cout<<"===== L1 contents =====\n";
			L1.output1(argv[8]);
			cout<<"\n===== L2 contents =====\n";
			L2.output1(argv[8]);
			cout<<"\n===== Simulation Results =====";
			cout<<std::setprecision(4)<<std::fixed;
			cout<<"\na. number of L1 reads:\t\t\t"<<L1.reads;
			cout<<"\nb. number of L1 read misses:\t\t"<<L1.rmiss;
			cout<<"\nc. number of L1 writes:\t\t\t"<<L1.writes;
			cout<<"\nd. number of L1 write misses:\t\t"<<L1.wmiss;
			float miss_rate=(L1.rmiss+L1.wmiss)/double(L1.reads+L1.writes);
			cout<<"\ne. L1 miss rate:\t\t\t"<<miss_rate;
			cout<<"\nf. number of writebacks from L1 memory:  "<<L1.write_back;
			cout<<"\ng. number of L2 reads:\t\t\t"<<L2.reads;
			cout<<"\nh. number of L2 read misses:\t\t"<<L2.rmiss;
			cout<<"\ni. number of L2 writes:\t\t\t"<<L2.writes;
			cout<<"\nj. number of L2 write misses:\t\t"<<L2.wmiss;
			miss_rate=(L2.rmiss)/double(L2.reads);
			//printf("\ne. L1 miss rate:\t\t\t")
			cout<<"\nk. L2 miss rate:\t\t\t"<<miss_rate;
			cout<<"\nl. number of writebacks from L2 memory:  "<<L2.write_back;
			cout<<"\nm. total memory traffic:\t\t"<<L2.rmiss+L2.wmiss+L2.write_back;
		}
		
		else if(arg[5]!=1 && arg[6]!=1 &&arg[3])
		{
			int address=0;
			//cache *p=NULL;
			cache L2(arg[0],arg[3],arg[4],arg[6],arg[5]);
			cache L1(arg[0],arg[1],arg[2],&L2);
			bool test;
			while(!fin.eof())
			{
				fin.getline(temp1,11,'\n');
				address=0;
				char *temp2=(temp1+2);
				//cout<<temp1<<"\n";	
				address=strtol(temp2,NULL,16);	
				if(temp1[0]=='r'||temp1[0]=='R')
				{
					L1.reads++;
					test=L1.readFromAddress(address);
					if(!test)
						L1.rmiss++;						
				}
				else if (temp1[0]=='w'||temp1[0]=='W')
				{
					L1.writes++;
					test=L1.writeToAddress(address);
					if(!test)	
						L1.wmiss++;
				}	
		//L1.output1(argv[8]);
		//L2.output2();
		//L2.output3();
	
			}	
	
			cout<<"===== Simulator configuration =====\n";
			cout<<"Blocksize:						  "<<arg[0]<<"\n";
			cout<<"L1_Size:					          "<<arg[1]<<"\n";
			cout<<"L1_ASSOC:						  "<<arg[2]<<"\n";	
			cout<<"L2_SIZE:						  "<<arg[3]<<"\n";
			cout<<"L2_ASSOC:						  "<<arg[4]<<"\n";	
			cout<<"L2_DATA_BLOCKS:						  "<<arg[5]<<"\n";
			cout<<"L2_ADDRESS_TAGS:					  "<<arg[6]<<"\n";
			cout<<"trace_file:                     \t \t\t  "<<argv[8]<<"\n\n";
			cout<<"===== L1 contents =====\n";
			L1.output1(argv[8]);
			cout<<"\n===== L2 Address Array contents =====\n";
			L2.output2();
			cout<<"\n===== L2 Data Array contents =====\n";
			L2.output3();
			cout<<"\n===== Simulation Results =====";
			cout<<std::setprecision(4)<<std::fixed;
			cout<<"\na. number of L1 reads:\t\t\t"<<L1.reads;
			cout<<"\nb. number of L1 read misses:\t\t"<<L1.rmiss;
			cout<<"\nc. number of L1 writes:\t\t\t"<<L1.writes;
			cout<<"\nd. number of L1 write misses:\t\t"<<L1.wmiss;
			float miss_rate=(L1.rmiss+L1.wmiss)/double(L1.reads+L1.writes);
			cout<<"\ne. L1 miss rate:\t\t\t"<<miss_rate;
			cout<<"\nf. number of writebacks from L1 memory:  "<<L1.write_back;
			cout<<"\ng. number of L2 reads:\t\t\t"<<L2.reads;
			cout<<"\nh. number of L2 read misses:\t\t"<<L2.rmiss;
			cout<<"\ni. number of L2 writes:\t\t\t"<<L2.writes;
			cout<<"\nj. number of L2 write misses:\t\t"<<L2.wmiss;
			cout<<"\nk. number of L2 sector misses:\t\t"<<L2.sector_miss;
			cout<<"\nl. number of L2 cache block misses:\t\t"<<L2.rmiss+L2.wmiss-L2.sector_miss;
			miss_rate=(L2.rmiss)/double(L2.reads);
			//printf("\ne. L1 miss rate:\t\t\t")
			cout<<"\nm. L2 miss rate:\t\t\t"<<miss_rate;
			cout<<"\nn. number of writebacks from L2 memory:  "<<L2.write_back;
			cout<<"\no. total memory traffic:\t\t"<<L2.rmiss+L2.wmiss+L2.write_back;
			
		}
		
		else if (arg[3]==0)
		{
			int address=0;
			cache *p=NULL;
			cache L1(arg[0],arg[1],arg[2],p);
			bool test;
			while(!fin.eof())
			{
				fin.getline(temp1,11,'\n');
				address=0;
				char *temp2=(temp1+2);	
				address=strtol(temp2,NULL,16);	
				if(temp1[0]=='r'||temp1[0]=='R')
				{
					L1.reads++;
					test=L1.readFromAddress(address);
					if(!test)
						L1.rmiss++;						
				}
				else if (temp1[0]=='w'||temp1[0]=='W')
				{
					L1.writes++;
					test=L1.writeToAddress(address);
					if(!test)	
						L1.wmiss++;
				}	
		
	
			}	
	
			cout<<"===== Simulator configuration =====\n";
			cout<<"Blocksize:						  "<<arg[0]<<"\n";
			cout<<"L1_Size:					          "<<arg[1]<<"\n";
			cout<<"L1_ASSOC:						  "<<arg[2]<<"\n";	
			cout<<"L2_SIZE:						  "<<arg[3]<<"\n";
			cout<<"L2_ASSOC:						  "<<arg[4]<<"\n";	
			cout<<"L2_DATA_BLOCKS:						  "<<arg[5]<<"\n";
			cout<<"L2_ADDRESS_TAGS:					  "<<arg[6]<<"\n";
			cout<<"trace_file:                     \t \t\t  "<<argv[8]<<"\n\n";
			cout<<"===== L1 contents =====\n";
			L1.output1(argv[8]);
			cout<<"\n===== Simulation Results =====";
			cout<<std::setprecision(4)<<std::fixed;
			cout<<"\na. number of L1 reads:\t\t\t"<<L1.reads;
			cout<<"\nb. number of L1 read misses:\t\t"<<L1.rmiss;
			cout<<"\nc. number of L1 writes:\t\t\t"<<L1.writes;
			cout<<"\nd. number of L1 write misses:\t\t"<<L1.wmiss;
			float miss_rate=(L1.rmiss+L1.wmiss)/double(L1.reads+L1.writes);
			cout<<"\ne. L1 miss rate:\t\t\t"<<miss_rate;
			cout<<"\nf. number of writebacks from L1 memory:  "<<L1.write_back;
			cout<<"\ng. total memory traffic:\t\t"<<L1.rmiss+L1.wmiss+L1.write_back;
		}
	
		
	
	}
	return 0;
}
