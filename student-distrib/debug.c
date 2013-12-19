#include "debug.h"
#include "fs.h"

#define num_dentries 16
#define DBEUG 0
/*	test_read_dentry_by_name
 *   DESCRIPTION: this function will test the function test_read_dentry_by_name
 *   INPUTS: adr -- the first address of the file system loaded in memory
*
 *   OUTPUTS: the files name and inode
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: fs_base_adr and num_xxx updated 
 */

void test_read_dentry_by_name(uint32_t * adr) {
	uint32_t i;
	for(i=0;i<num_dentries;i++) {
		dentry_t test;
		dentry_t* cur_dentry = (dentry_t*)(adr)+i+1;
		uint8_t* fname = cur_dentry->fname;
		
		read_dentry_by_name( fname,&test);

		printf("name: %s type: %d \n",test.fname,test.type);
		ASSERT(val == 0);
		ASSERT(&test != NULL);
		ASSERT(test.type == 1);

		printf("index node: %d\n",test.inode);
	}
}

/*	test_read_dentry_by_index
 *   DESCRIPTION: this function will test the function test_read_dentry_by_index
 *   INPUTS: adr -- the first address of the file system loaded in memory
*
 *   OUTPUTS: the files name and inode
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: fs_base_adr and num_xxx updated 
 */
void test_read_dentry_by_index (uint32_t * adr) {
	uint32_t i;
	for(i=0;i<num_dentries;i++) {
		dentry_t test;
		
		read_dentry_by_index(i,&test);

		printf("name: %s type: %d \n",test.fname,test.type);
		ASSERT(val == 0);
		ASSERT(&test != NULL);
		ASSERT(test.type == 1);

		printf("index node: %d\n",test.inode);
	}
	
}
/*	test_read_data
 *   DESCRIPTION: this function will test read_data function
 *   INPUTS: NONE
*
 *   OUTPUTS: the data of the file
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: NONE 
 */
void test_read_data () {
	dentry_t test;

	uint8_t* fname =(uint8_t*)"cat";

	read_dentry_by_name( fname,&test);
	ASSERT(0);
	ASSERT(&test != NULL);
	ASSERT(test.type == 1);
	printf("name: %s type: %d index node: %d\n",test.fname,test.type,test.inode);

	uint8_t buf[5277];


	uint32_t t = read_data (test.inode, 0, buf, 5277) ;

	printf("val:%d file:\n%s \n",t,buf);
	
}
/*	test_read_dir
 *   DESCRIPTION: this function will test read_dir function
 *   INPUTS: NONE
*
 *   OUTPUTS: the data of the directory
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: NONE 
 */
void test_read_dir () {
#if(DEBUG == 1)
	int32_t cnt;
	uint8_t* fname =(uint8_t*) ".";
	uint8_t buf[33];

	dir_open(fname); 

	 while (0 != (cnt = dir_read (fname, buf, 32))) {
        if (-1 == cnt) {
	        printf("directory entry read failed\n");
	    }
	    buf[cnt] = '\n';
	   	printf("%s \n",buf);
    }
#endif
	
}
/*	test_read_file
 *   DESCRIPTION: this function will test read_file function
 *   INPUTS: NONE
 *   OUTPUTS: content of the file
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: NONE 
 */
void test_read_file () {
#if(DEBUG == 1)
	int32_t cnt;
	uint8_t* fname =(uint8_t*) "frame1.txt";
	uint8_t buf[1000];

	file_open(fname); 

	while (0 != (cnt = file_read(fname, buf, 500))) {
        if (-1 == cnt) {
	        printf("file read failed\n");
	    }
	    buf[cnt] = '\n';
	   	printf("cnt:%d file:\n%s \n",cnt,buf);
    }
#endif
}
