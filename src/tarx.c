// Lab 5: tarx.c
// This lab will mimic the extract functionality of the 
//      program tar, which unbundles files and directories.
//      http://web.eecs.utk.edu/~jplank/plank/classes/cs360/360/labs/Lab-6-Tarx/index.html
//      for more information and lab specifications. 

// COSC 360
// Swasti Mishra 
// Mar 28, 2022
// James Plank 

// Libraries
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <sys/stat.h>
# include <sys/time.h>

// Plank Libraries
# include "jrb.h"
# include "dllist.h"

// Compares jvals 
int compare( Jval val1, Jval val2 ){
    if( val1.l < val2.l ){
        return -1 ;
    }
    if( val1.l > val2.l ){
        return 1 ;
    }
    return 0 ;
}

// Reads in the file name size
//		There's no reason why I couldn't just put this in the 
//		main function but for some reason it was bugging out?
int readFNSize(){
	int FNSize = 0 ;
	if( fread( &FNSize, sizeof(int), 1, stdin ) == 0 ){
		return -1 ;
	}
	else{
		return FNSize ;
	}
}

int main(){
	// The tree will store all of our inodes, and the lists 
	//		will store all of the information about the files. 
	JRB tree = make_jrb() ;
	Dllist FNList   = new_dllist() ;
	Dllist modeList = new_dllist() ;
	Dllist modTimeList = new_dllist() ;
	
	// Read from standard input
	while(1){
		char temp = 0 ;
		
		// Reads the file name size
		int FNSize = readFNSize() ;

		// Breaks while(1) loop
		if( FNSize == -1 ){
			break ;
		}
		
		// Reads filename and sets to NULL, error checks, reads bytes incrementally
		char *FN = ( char* ) malloc( FNSize * sizeof(char) + 1 ) ;
		FN[FNSize] = '\0' ;
		for( int i = 0 ; i < FNSize ; i++ ){
			if( fread( &temp, 1, 1, stdin ) == 0 ){
				fprintf( stderr, "Error reading in file name\n" ) ;
				exit(1) ;
			}
			FN[i] = temp ;
		}

		// Reads the inode number, error checks
		long inode = 0 ;
		if( fread( &inode, sizeof(long), 1, stdin ) == 0 ){
			fprintf( stderr, "Couldn't read inode\n" ) ;
			exit(1) ;
		}

		// Checks if inode is in tree first
		JRB inodeSearch = jrb_find_gen( tree, new_jval_l(inode), compare ) ;

		// If it's not in there, go ahead and add it
		if( inodeSearch == NULL ){
			jrb_insert_gen( tree, new_jval_l(inode), new_jval_s( strdup(FN) ), compare ) ;
		}
        // Otherwise, link to the file
		else{
			link( inodeSearch->val.s, FN ) ;
			continue ;
		}

		// Reads mode
		int mode = 0 ;
		fread( &mode, sizeof(int), 1, stdin ) ;
	
		// Reads last mod time
		long modTime = 0 ;
		if( fread( &modTime, sizeof(long), 1, stdin ) == 0 ){
			fprintf( stderr, "Error reading modTime\n" ) ;
			exit(1) ;
		}

		// If it exists, create directory 
		if( S_ISDIR(mode) ){
			if( mkdir( FN, 0777 ) == -1 ){
				// Load bearing empty if statement
			}
		}
		else {
            // Otherwise, read bytes and error check
			long fileSize = 0 ;
			if( fread( &fileSize, sizeof(long), 1, stdin ) == 0 ){
				fprintf( stderr, "Couldn't read file size\n" ) ;
				exit(1) ;
			}
			char temp ;
			char *bytes = ( char* ) malloc( fileSize * sizeof(char) + 1 ) ;

			// Reads bytes incrementally
			for ( int i = 0 ; i < fileSize ; i++ ){
				if( fread( &temp, 1, 1, stdin ) == 0 ){
					fprintf( stderr, "Error reading bytes\n" ) ;
					exit(1) ;
				}
				bytes[i] = temp ;
			}

			// Write to the file and free memory/close file 
			FILE *file = fopen( FN, "w" ) ;
			fwrite( bytes, 1, fileSize, file ) ;
			free(bytes) ;
			fclose(file) ;
		}

		// Timeval stuff
		struct timeval *timeArray = ( struct timeval* ) malloc( sizeof(struct timeval) * 2 ) ;
		gettimeofday( &timeArray[0], NULL ) ;
		timeArray[0].tv_usec = 0 ;
		timeArray[1].tv_usec = 0 ;
		timeArray[1].tv_sec = modTime ;
		
		// Add timeval info to lists
		dll_append( modeList, new_jval_i(mode) ) ;
		dll_append( FNList, new_jval_s( strdup(FN) ) ) ;
		dll_append( modTimeList, new_jval_v( (void *) timeArray ) ) ;
	}

	// Iterators so we can traverse
	Dllist FNIterator   = dll_last( FNList ) ;
	Dllist modeIterator = dll_last( modeList ) ;
	Dllist timeIterator = dll_last( modTimeList ) ;

	// Set modtimes and chmod
	while( timeIterator != dll_nil(modTimeList) && dll_nil(FNList) != FNIterator ){
		struct timeval *tempStruct = ( struct timeval* ) timeIterator->val.v ;
		if ( utimes( FNIterator->val.s, tempStruct ) == -1 ){
			exit(1) ;
		}
		chmod( FNIterator->val.s, modeIterator->val.i ) ;
		FNIterator   = FNIterator->blink ;
		timeIterator = timeIterator->blink ;
		modeIterator = modeIterator->blink ;
	}

	// Freeing memory 
	Dllist temp ;
	dll_traverse( temp, FNList ) ;
	free( temp->val.s ) ;
	dll_traverse( temp, modTimeList ){
		struct timeval *tempStruct = ( struct timeval* ) timeIterator->val.v ;
		free(tempStruct) ;
	}

	// Free up the lists
	free_dllist(FNList) ;
	free_dllist(modTimeList) ;
	free_dllist(modeIterator) ;

	return 0 ;
}
