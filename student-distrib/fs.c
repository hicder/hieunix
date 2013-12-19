#include "fs.h"
#include "syscalls.h"

uint32_t* fs_base_adr = 0;	// the start address of the file system

/*variables containing infos about the file system*/
uint32_t num_dentries;
uint32_t num_index_nodes;
uint32_t num_data_blocks;

/* directory buffer, length and offset */
uint8_t dir_buf[MAX_BUF_SIZE];
uint32_t dir_length;

uint32_t get_file_length(unsigned int inode){
	uint32_t* node = (uint32_t*) ((uint8_t *)fs_base_adr + (inode+1)*BLOCK_SIZE);	//index nodes start at 1st entry in file system
	uint32_t file_length = node[0];		// file length
   	return file_length;
}

/*	fs_init
 *   DESCRIPTION: this function will initialize the file system
 *   INPUTS: adr -- the first address of the file system loaded in memory
*
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: fs_base_adr and num_xxx updated 
 */
void fs_init (uint32_t* adr) {
	if(!adr) {
		//test
		printf("file system pointer is invalid"); 
	}

	fs_base_adr = adr;	// store the start address 
	
	/* get the infos from the boot lock*/
	num_dentries = fs_base_adr[0];
	num_index_nodes =fs_base_adr[1];
	num_data_blocks = fs_base_adr[2];

	//test
	printf("num_dentries: %d, num_index_nodes: %d, num_data_blocks: %d \n",num_dentries,num_index_nodes,num_data_blocks); 
}

/*	file_open
 *   DESCRIPTION: this function opens a file in the directory by checking it's the right type
 *   INPUTS: fname -- file name
 *   OUTPUTS: NONE
 *   RETURN VALUE: 0 if is file type, -1 if type is invalid
 *   SIDE EFFECTS: NONE
 */
int32_t file_open (fd_t*  file_desc, const uint8_t* fname) {

	dentry_t f_entry;
	// check if file existes and is regular file
	if(read_dentry_by_name(fname,&f_entry) == 0 ) {
		if(f_entry.type == REG_FILE && file_desc != NULL) {

			file_desc->fops_p = &file_fops; // in syscall
			file_desc->inode_p = (uint32_t*) ((uint8_t *)fs_base_adr + (f_entry.inode+1)*BLOCK_SIZE);
			file_desc->file_pos = 0;	   
			file_desc->flags = 1;

			return 0;
		}
	}

	return -1; // file does not exist or not a regular file
}

/*	dir_read
 *   DESCRIPTION: this function reads the names of all files in the directory
 *   INPUTS: fname -- file name
 *			 buf   -- buffer to write to
 *			 length -- number of bytes to write
 *   OUTPUTS: write the file names into buf
 *   RETURN VALUE: number of bytes written, 0 if end of file reached
 *   SIDE EFFECTS: buf is modified
 */
int32_t file_read(fd_t*  file_desc, uint8_t* buf, uint32_t nbytes) {

	uint32_t node;
	uint32_t bytes_read;

	if( !file_desc || file_desc->flags != 1 || !file_desc->inode_p) {
		return -1;	// file not in use or file is a directorys
	}

	node = (uint32_t)(( (uint32_t)file_desc->inode_p - (uint32_t)fs_base_adr) / BLOCK_SIZE -1); // index of inode 
	bytes_read = read_data (node, file_desc->file_pos, buf, nbytes);	//read data from file

	file_desc->file_pos += bytes_read;	//update the file position

	return bytes_read;
}

/*	 file_write
 *   DESCRIPTION: this function will always return -1 since the file system is read only
 *   INPUTS: fname -- file name
 *			 buf   -- buffer to write to
 *			 length -- number of bytes to write
 *   OUTPUTS: NONE
 *   RETURN VALUE: always return -1
 *   SIDE EFFECTS: NONE
 */
int32_t file_write(fd_t* file_desc,const uint8_t* buf, uint32_t nbytes) {
	// read only file system
	// always returns -1
	return -1;	
}

/*	 file_close
 *   DESCRIPTION: this function is called when a file is closed
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   RETURN VALUE: always return 0
 *   SIDE EFFECTS: NONE
 */
int32_t file_close(fd_t* file_desc) {

	if(!file_desc) {
		return -1;
	}
	// Set the file descriptor to NULL
	file_desc->fops_p = NULL;
	file_desc->inode_p = NULL;
	file_desc->file_pos = 0;	   
	file_desc->flags = 0;	

	return 0;
}

/*	 dir_open
 *   DESCRIPTION: this function opens a directory and stores all file names into dir_buf
 *   INPUTS: fname -- file name
 *   OUTPUTS: dir_buf is filled with all file names, each taking 32 bytes
 *   RETURN VALUE: 0 if is directory type, -1 if type is invalid
 *   SIDE EFFECTS: NONE
 */
int32_t dir_open(fd_t* file_desc,const uint8_t* fname) {

	dentry_t d_entry;
	uint32_t i,j;

	if( read_dentry_by_name(fname,&d_entry) == 0 ) {
		if(d_entry.type == DIR_FILE && file_desc != NULL) {		// check file type is directory
			
			file_desc->fops_p = &dir_fops; // in syscall // in syscall // in syscall
			file_desc->inode_p = NULL;
			file_desc->file_pos = 0;	   
			file_desc->flags = 1;

			dir_length = 0;
			// store all the file names into dir_buf
			for(i=0;i<num_dentries;i++) {
				if(read_dentry_by_index (i,&d_entry) == 0) {
					// copy the file name into dir_buf
					strncpy( (int8_t*)(dir_buf+dir_length), (int8_t*)d_entry.fname,FNAME_SIZE);	

					// file names are not exactly 32 bytes, we add null characters to fill up 32 bytes
					for(j=strlen((int8_t *)dir_buf);j<(i+1)*FNAME_SIZE;j++) {
						dir_buf[dir_length+FNAME_SIZE+j] = '\0';
					}
					// length increment by file name size
					dir_length += FNAME_SIZE;
				}
			}

			return 0; // success
		}
	}

	// file not exist or not a directory
	return -1;
}



/*	dir_read
 *   DESCRIPTION: this function reads the names of all files in the directory
 *   INPUTS: fname -- file name
 *			 buf   -- buffer to write to
 *			 length -- number of bytes to write
 *   OUTPUTS: write the file names into buf
 *   RETURN VALUE: number of bytes written, 0 if end of file reached
 *   SIDE EFFECTS: buf is modified
 */
int32_t dir_read(fd_t* file_desc, uint8_t* buf, uint32_t nbytes) {

	if(!file_desc) {
		return -1;	// inode should be null for directory
	}

	uint32_t i;
	uint32_t end_of_file = file_desc->file_pos >= dir_length;	//checks if end of file reached

	// read to the end of file or end of buffer
	for(i=0;i<nbytes && (!end_of_file);i++) {
		buf[i] = dir_buf[file_desc->file_pos+i];					// copy to buf
		end_of_file = (file_desc->file_pos+i+1) >= dir_length;	// check if is end of file	
	}

	if(end_of_file) {
		file_desc->file_pos = dir_length;
		return 0;	//end of file reached
	}

	file_desc->file_pos += nbytes;	//update current offset
	return i;
}


/*	dir_write
 *   DESCRIPTION: this function will always return -1 since the file system is read only
 *   INPUTS: fname -- file name
 *			 buf   -- buffer to write to
 *			 length -- number of bytes to write
 *   OUTPUTS: NONE
 *   RETURN VALUE: always return -1
 *   SIDE EFFECTS: NONE
 */
int32_t dir_write(fd_t* file_desc,const uint8_t* buf, uint32_t nbytes) {

	return -1;	
}


/*	dir_close
 *   DESCRIPTION: this function is called when directory is closed
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   RETURN VALUE: always return 0
 *   SIDE EFFECTS: NONE
 */
int32_t dir_close(fd_t* file_desc) {

	if(!file_desc) {
		return -1;
	}

	file_desc->fops_p = NULL;
	file_desc->inode_p = NULL;
	file_desc->file_pos = 0;	   
	file_desc->flags = 0;	

	return 0;
}






/*	read_dentry_by_name
 *   DESCRIPTION: this function will fill in dentry struct of a given file on success
 *   INPUTS: fname -- the name of file to read
 *   	     dentry -- directory struct which contains the name, type and index for the file 
 *   OUTPUTS: dentry with filled values on success
 *   RETURN VALUE: 0 on success, -1 with non-existent file name
 *   SIDE EFFECTS: dentry modified
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry) {

	int32_t i;
	if(!fs_base_adr) {
		return -1; // not initialized
	}

	if(!dentry) {
		return -1;	// null pointer 
	}

	for(i=0;i<num_dentries;i++) {
		//get current directory entry, dentries start at 1st entry in boot block
		dentry_t* cur_dentry = (dentry_t*)(fs_base_adr)+i+1;

		// file found
		if( strncmp( (int8_t*)cur_dentry->fname,(int8_t*)fname, FNAME_SIZE) == 0 ) {   // does strncmp work??
			//update the dentry with found file
			strncpy( (int8_t*)dentry->fname, (int8_t*)cur_dentry->fname,FNAME_SIZE);
			dentry->type = cur_dentry->type;
			dentry->inode = cur_dentry->inode;

			return 0;	// return on success
		}	
		
		
	}

	return -1; // fname does not exist 
}



/*	read_dentry_by_index
 *   DESCRIPTION: this function will fill in dentry struct of a given file on success
 *   INPUTS: index -- the index of the file to read
 *   	     dentry -- directory struct which contains the name, type and index for the file 
 *   OUTPUTS: dentry with filled values on success
 *   RETURN VALUE: 0 on success, -1 with invalid index
 *   SIDE EFFECTS: dentry modified
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry) {
	if(!fs_base_adr) {
		return -1; // not initialized
	}

	if(index < 0 || index >= num_dentries) {
		return -1;	//invalid index
	}

	if(!dentry) {
		return -1;	// null pointer 
	}

	//get current directory entry, dentries start at 1st entry in boot block
	dentry_t* cur_dentry =(dentry_t*) (fs_base_adr)+index+1; 

	//update the dentry with found file
	strcpy( (int8_t*)dentry->fname, (int8_t*)cur_dentry->fname);
	dentry->type = cur_dentry->type;
	dentry->inode = cur_dentry->inode;	

	return 0; // index does not exist 
}


/*	read_data
 *   DESCRIPTION: this function will read data from file system on success
 *   INPUTS: inode -- the index node number of a file to read
 *   	     offset -- the offset within index node to start
 * 		 	 buf --	pointer to store data into
 *			 length -- length of bytes to read
 *   OUTPUTS: data from the file
 *   RETURN VALUE: 0 on success, -1 with invalid inode or bad data block number
 *   SIDE EFFECTS: buffer modified
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {

	if(!fs_base_adr) {
		return -1; // not initialized
	}

	if(inode < 0 || inode >= num_index_nodes) {
		return -1;	//invalid index node number
	}

	if(!buf) {
		return -1;	// null pointer 
	}

	if(offset <0 ) {
		return -1;	//invalid offset
	}
	memset(buf, 0, length);
	uint32_t* node = (uint32_t*) ((uint8_t *)fs_base_adr + (inode+1)*BLOCK_SIZE);	//index nodes start at 1st entry in file system
	uint32_t file_length = node[0];		// file length

	uint32_t block_index;	// index in file_block
	uint32_t block_number; 	// data block number in file system
	uint8_t* cur = 0;		//pointer to current byte being read
	uint32_t i;

	uint32_t end_of_file = offset >= file_length;	//checks if end of file reached
	if(end_of_file) {
		return 0;	//end of file reached
	}

	// read to the end of file or end of buffer
	for(i=0;i<length && (!end_of_file);i++) {

		// switch to next data block if cur reaches end of data block	
		block_index = (offset+i) / BLOCK_SIZE;  		// index of current data block in node
		block_number = node[block_index+1];		// data block number

		if(block_number < 0 || block_number >= num_data_blocks) {
			return -1;	//bad data block number
		}	

		// get current pointer to the data being read
		cur = (uint8_t*)fs_base_adr + (num_index_nodes+1+block_number)*BLOCK_SIZE + ((offset+i) % BLOCK_SIZE);
		
		buf[i] = *cur;	//copy data to buffer
		cur++;	
		end_of_file = (offset+i + 1) >= file_length;	// check if is end of file	
	}

	return i;	//number of bytes read
}






