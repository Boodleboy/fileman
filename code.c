#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <sys/stat.h>

#define PATH_MAX 4096


struct file {
	char name[260];
	time_t t;
	off_t size;
};

struct folder {
	char path[PATH_MAX];
	bool isinit;
	int screenpos;
	int cursorpos;
	int numfolders;
	int numfiles;
	struct folder *parent;
	struct folder *subfolders;
	struct file *files;
};

struct folder *current_folder;

size_t charsize;
size_t direntsize;
size_t pathsize;
size_t entrysize;
size_t foldersize;
size_t filesize;


void dirinit(struct folder *myfolder);
void redraw();
void parent_dir_name(char dir[], char parent[]);
int dir_layers(char path[]);
void parent_layer(int layers, char dest[], char src[]); 
void getlastpart(char dest[], char src[]);
void sortsubs(struct folder *myfolder, char type);

void dirinit(struct folder *myfolder){
//	printf("initializing %s\n",myfolder->path);
	myfolder->screenpos = 0;
	myfolder->cursorpos = 0;
	
	myfolder->numfolders = 0;
	myfolder->numfiles = 0;

	bool morefiles = true;
	struct dirent *myentry;
	struct stat mystat;
	DIR *mydir = opendir(myfolder->path);
	char mypath[PATH_MAX];

	
	while (morefiles) {
		myentry = readdir(mydir);
		if (myentry != NULL){
			if (strcmp(myentry->d_name,".") != 0 && strcmp(myentry->d_name,"..") != 0){
				strcpy(mypath, myfolder->path);
				if (strcmp(mypath,"/") != 0){
					strcat(mypath, "/");
				}
				strcat(mypath, myentry->d_name);
				stat(mypath,&mystat);


				if (S_ISDIR(mystat.st_mode)){
				       myfolder->numfolders += 1;
				} 
				if (S_ISREG(mystat.st_mode)){
					myfolder->numfiles += 1;
				}	
			}
		}
		else {
			morefiles = false;
		}
	}

	closedir(mydir);
	mydir = opendir(myfolder->path);
	morefiles = true;
	myfolder->subfolders = malloc(foldersize * myfolder->numfolders);
	myfolder->files = malloc(filesize * myfolder->numfiles);
	
	char holdstr[PATH_MAX];
	int i=0, j=0;

	int mycounter = 0;

	while (morefiles) {
		myentry = readdir(mydir);
		if (myentry != NULL){
			if (strcmp(myentry->d_name,".") != 0 && strcmp(myentry->d_name,"..") !=0){
				strcpy(mypath, myfolder->path);
				if (strcmp(mypath,"/") != 0){
					strcat(mypath,"/");
				}
				strcat(mypath, myentry->d_name);
				stat(mypath,&mystat);

				if (S_ISDIR(mystat.st_mode)){
					strcpy(myfolder->subfolders[i].path,myfolder->path);
					if (strcmp("/",myfolder->path) != 0) {
						strcat(myfolder->subfolders[i].path,"/");
					}
					strcat(myfolder->subfolders[i].path,myentry->d_name);
					myfolder->subfolders[i].isinit = false;
					myfolder->subfolders[i].parent = myfolder;
					i++;
				} else if (S_ISREG(mystat.st_mode)){
					strcpy(myfolder->files[j].name,myentry->d_name);
					j++;
				}
			}	
		} else {
			morefiles = false;
		}
	}

	sortsubs(myfolder,'p');

	myfolder->isinit = true;
}


void redraw(){

	clear();
	char holder[260];
	struct folder *myfolder = current_folder->parent;
	int numlines = LINES;
	int col2start = COLS / 3;
	int col3start = 2 * col2start;
	int startpos = 1;

	if (current_folder->parent != NULL){
		int numentries = myfolder->numfolders + myfolder->numfiles;
		for (int i=0;i < numlines && i + myfolder->screenpos < myfolder->numfolders; i++){
			if (i==current_folder->parent->cursorpos) {
				attron(A_STANDOUT);
				getlastpart(holder, myfolder->subfolders[i].path);
				mvaddstr(i+startpos,0,holder);
				attroff(A_STANDOUT);
			}
			else {
				getlastpart(holder, myfolder->subfolders[i].path);
				mvaddstr(i+startpos,0,holder);
			}
		}
	}
	myfolder = current_folder;
	for (int i=0;i < LINES && i + myfolder->screenpos < myfolder->numfolders; i++){
		if (i==current_folder->cursorpos) {
			attron(A_STANDOUT);
			getlastpart(holder, myfolder->subfolders[i].path);
			mvaddstr(i+1,15,holder);
			attroff(A_STANDOUT);
		}
		else {
			getlastpart(holder, myfolder->subfolders[i].path);
			mvaddstr(i+1,15,holder);
		}
	}
	mvaddstr(0,0,current_folder->path);
	refresh();
}    

int dir_layers(char path[]){

	int num_layers = 1;

	for (int i=1;path[i] != '\0';i++){
		if (path[i] == '/' && path[i-1] != '\\'){
			num_layers++;
		}
	}
	return num_layers;
}

void parent_layer(int layer, char dest[], char src[]){
	
	strcpy(dest,src);
	int j = 1, i=1;
	while (i < layer){
		if (dest[j] == '/' && dest[j-1] != '\\'){
			i++;
		} else if (dest[j] == '\0'){
			printf("error: not enough layers");
		}
		j++;
	}
	dest[j-1] = '\0';
}

void getlastpart(char dest[], char src[]){
	int mylen = strlen(src);
	char *myptr;

	for (int i = mylen - 1; i >= 0; i--) {
		if (src[i] == '/' && src[i-1] != '\\'){
			myptr = &src[i+1];
			break;
		}
	}
	strcpy(dest, myptr);
}

void sortsubs(struct folder *myfolder, char type){

	struct folder holder;
	for (int i=1; i<myfolder->numfolders; i++){
		for (int j=1; j<myfolder->numfolders; j++){
			if (strcmp(myfolder->subfolders[j].path,myfolder->subfolders[j-1].path) < 0){
				holder = myfolder->subfolders[j];
				myfolder->subfolders[j] = myfolder->subfolders[j-1];
				myfolder->subfolders[j-1] = holder;
			}
		}
	}

}


void parent_dir_name(char dir[], char parent[]){
	int length = strlen(dir);
	strcpy(parent,dir);
	for (int i=length-1;i>-1;i--){
		if (parent[i] == '/') {
			if (i == 0){
				parent[1] = '\0';
				break;
			} else {
				parent[i] = '\0';
				break;
			}
		}
	}
}


int main(){
	
	initscr();
	clear();
	noecho();

	char myinput;
	bool continuing = true;
	char mypath[PATH_MAX];

	charsize = sizeof(char);
	pathsize = charsize * PATH_MAX;
	direntsize = sizeof(struct dirent);
	foldersize = sizeof(struct folder);
	filesize = sizeof(struct file);


	getcwd(mypath, pathsize);
	int layers = dir_layers(mypath);

	struct folder rootdir;
	rootdir.isinit = false;
	strcpy(rootdir.path,"/");
	rootdir.parent = NULL;
	dirinit(&rootdir);

	char curpath[PATH_MAX];
	struct folder *curdir = &rootdir;
//	printf("layers = %d\n",layers);
//	printf("path is %s\n", mypath);


	for (int i=2;i <= layers; i++){
		parent_layer(i, curpath, mypath);
//		printf("curdir name is %s\n", curdir->path);
//		printf("curdir numfolders is %d\n", curdir->numfolders);
//		printf("looking for subfolder %s\n\n\n", curpath);
		for (int j=0; j < curdir->numfolders; j++){
//			printf("path being searched is: %s\n",curdir->subfolders[j].path);
			if (strcmp(curpath,curdir->subfolders[j].path) == 0){
//				printf("found %s\n",curpath);
				dirinit(&(curdir->subfolders[j]));
				curdir = &(curdir->subfolders[j]);
//				printf("new curdir is %s\n",curdir->path);
//				printf("new curdir numfolders is %d\n\n", curdir->numfolders);
				break;
			}
		}
	}
	current_folder = curdir;


	int folderindex;

	redraw();

	while (continuing){
		myinput = getch();
		switch (myinput)
		{
			case 'q':
				continuing = false;
				break;
			case 'j':
				current_folder->cursorpos += 1;
				break;
			case 'k':
				if (current_folder->cursorpos > 0){
					current_folder->cursorpos -= 1;
				}
				break;
			case 'h':
				if (current_folder->parent != NULL){
					current_folder = current_folder->parent;
				}
				break;
			case 'l': 
				folderindex = current_folder->cursorpos + current_folder->screenpos;
				current_folder = &(current_folder->subfolders[folderindex]);
				if (!(current_folder->isinit)){
					dirinit(current_folder);
				}
				break;

		}

		redraw();
	}
	
	endwin();
	return 0;
}
