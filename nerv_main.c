#define FUSE_USE_VERSION 31
#define _GNU_SOURCE

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#define pathMAX         1000
#define pathLenMax      100
#define maxFile         1000
#define chaterMax       100
#define chaterNameMax   100
#define regFile         "/register_bot"
#define chatRoomFile    "/chatRoom"

int logCnt=0;
void printLog(const char* funcName,const char* path){
    printf("[log %d] %s: %s\n",logCnt,funcName,path);
    logCnt++;
}

void printLogInt(const char* funcName,const char* path1,const int value){
    printf("[log %d] %s: %s %d\n",logCnt,funcName,path1,value);
    logCnt++;
}

struct nodeInfo{
    int isDir;
    char selfPath[pathLenMax+1];
    char ctx[maxFile+1];
    int chat_times;
};

char pathList[pathMAX+1][pathLenMax+1];
struct nodeInfo infoList[pathMAX+1];
struct nodeInfo* tmpNodeList[pathMAX+1];
char chater[chaterMax][chaterNameMax];

struct nodeInfo* getNodeInfo(const char* path){
    for(int i=0;i<pathMAX;i++){
        if(strcmp(path,pathList[i])==0)
            return infoList+i;
    }
    return NULL;
}

int getNodeIndex(const char* path){
    for(int i=0;i<pathMAX;i++){
        if(strcmp(path,pathList[i])==0)
            return i;
    }
    return -1;
}

int modifyNodeName(const char* oriPath,const char* newPath){
    int oriIndex=-1;
    for(int i=0;i<pathMAX;i++){
        if(strcmp(oriPath,pathList[i])==0){
            oriIndex=i;
            break;
        }
    }
    if(oriIndex==-1)
        return -ENOENT;
    strcpy(pathList[oriIndex],newPath);
    return 0;
}

int swapNode(const char* oriPath,const char* newPath){
    int p1=-1,p2=-1;
    for(int i=0;i<pathMAX;i++){
        if(strcmp(oriPath,pathList[i])==0)
            p1=i;
        if(strcmp(newPath,pathList[i])==0)
            p2=i;
        if(p1!=-1 && p2!=-1)
            break;
    }
    if(p1==-1 || p2==-1)
        return -ENOENT;
    char tmp[pathLenMax+1];
    strcpy(tmp,pathList[p1]);
    strcpy(pathList[p1],pathList[p2]);
    strcpy(pathList[p2],tmp);
    return 0;
}

int getSonNode(const char* path){
    int oriLen=strlen(path);
    int tmpCnt=0;

    for(int i=0;i<pathMAX;i++){
        int tmpLen=strlen(pathList[i]);
        if(oriLen<tmpLen && strncmp(path,pathList[i],oriLen)==0){
            if(strrchr(pathList[i]+oriLen+1,'/')==NULL){
                tmpNodeList[tmpCnt]=infoList+i;
                tmpCnt++;
            }
        }
    }
    tmpNodeList[tmpCnt]=NULL;
    return tmpCnt;
}

struct nodeInfo* addNode(const char* path){
    int empty_pos=-1;
    for(int i=0;i<pathMAX;i++){
        if(pathList[i][0]=='\0'){
            empty_pos=i;
            break;
        }
    }
    if(empty_pos==-1 || getNodeInfo(path)!=NULL)
        return NULL;
    strcpy(pathList[empty_pos],path);
    strcpy(infoList[empty_pos].selfPath,path);
    infoList[empty_pos].ctx[0]='\0';
    infoList[empty_pos].chat_times=0;
    return infoList+empty_pos;
}

int rmNode(const char* path){
    for(int i=0;i<pathMAX;i++){
        if(strcmp(path,pathList[i])==0){
            strcpy(pathList[i],"\0");
            return 0;
        }
    }
    return -ENOENT;
}

int isChater(const char* path){
    if(strcmp(regFile,path)==0)
        return 2;
    if(strncmp(regFile,path,strlen(regFile))==0 && path[strlen(regFile)]=='/')
        return 1;
    else
        return 0;
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored. The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given. In that case it is passed to userspace,
 * but libfuse and the kernel will still assign a different
 * inode for internal use (called the "nodeid").
 *
 * `fi` will always be NULL if the file is not currently open, but
 * may also be NULL if the file is open.
 */
int nerv_getattr (const char *path, struct stat *stbuf, struct fuse_file_info *fi){
    printLog("getattr",path);

    (void) fi;
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
    struct nodeInfo* info=getNodeInfo(path);
    if (info==NULL) return -ENOENT;
	if (info->isDir==1) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(info->ctx);
	}
	return res;
}

int _touchFile_(const char* path){
    struct nodeInfo* node=addNode(path);
    if(node==NULL)
        return -EEXIST;
    node->isDir=0;
    if(isChater(path)==1){
        const char* cName=path+strlen(regFile)+1;
        struct nodeInfo* chatroomNode=getNodeInfo(chatRoomFile);
        strcat(chatroomNode->ctx,"welcome ");
        strcat(chatroomNode->ctx,cName);
        strcat(chatroomNode->ctx," to the chat room!\n");
    }
    return 0;
}

/** Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */
int nerv_mknod (const char * path, mode_t mode, dev_t dev){
    printLog("mknod",path);

    (void) mode, (void) dev;
    return _touchFile_(path);
}

int _mkdir_(const char *path){
    struct nodeInfo* node=addNode(path);
    if(node==NULL)
        return -EEXIST;
    if(isChater(path)==1)
        return -EPERM;
    node->isDir=1;
    return 0;
}

/** Create a directory
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits use  mode|S_IFDIR
 * */
int nerv_mkdir (const char *path, mode_t mode){
    printLog("mkdir",path);
    (void) mode;
    return _mkdir_(path);
}

/** Remove a file */
int nerv_unlink (const char *path){
    printLog("unlink",path);

    struct nodeInfo* node=getNodeInfo(path);
    if(node==NULL)
        return -ENOENT;
    else if(node->isDir==1)
        return -EISDIR;
    else if(strcmp(path,chatRoomFile)==0)
        return -EPERM;
    else
        return rmNode(path);
}

/** Remove a directory */
int nerv_rmdir (const char *path){
    printLog("rmdir",path);

    struct nodeInfo* node=getNodeInfo(path);
    if(node==NULL)
        return -ENOENT;
    else if(node->isDir==0)
        return -ENOTDIR;
    else if(isChater(path)!=0)
        return -EPERM;
    else
        return rmNode(path);
}

/** Open a file
 *
 * Open flags are available in fi->flags. The following rules
 * apply.
 *
 *  - Creation (O_CREAT, O_EXCL, O_NOCTTY) flags will be
 *    filtered out / handled by the kernel.
 *
 *  - Access modes (O_RDONLY, O_WRONLY, O_RDWR, O_EXEC, O_SEARCH)
 *    should be used by the filesystem to check if the operation is
 *    permitted.  If the ``-o default_permissions`` mount option is
 *    given, this check is already done by the kernel before calling
 *    open() and may thus be omitted by the filesystem.
 *
 *  - When writeback caching is enabled, the kernel may send
 *    read requests even for files opened with O_WRONLY. The
 *    filesystem should be prepared to handle this.
 *
 *  - When writeback caching is disabled, the filesystem is
 *    expected to properly handle the O_APPEND flag and ensure
 *    that each write is appending to the end of the file.
 * 
     *  - When writeback caching is enabled, the kernel will
 *    handle O_APPEND. However, unless all changes to the file
 *    come through the kernel this will not work reliably. The
 *    filesystem should thus either ignore the O_APPEND flag
 *    (and let the kernel handle it), or return an error
 *    (indicating that reliably O_APPEND is not available).
 *
 * Filesystem may store an arbitrary file handle (pointer,
 * index, etc) in fi->fh, and use this in other all other file
 * operations (read, write, flush, release, fsync).
 *
 * Filesystem may also implement stateless file I/O and not store
 * anything in fi->fh.
 *
 * There are also some flags (direct_io, keep_cache) which the
 * filesystem may set in fi, to change the way the file is opened.
 * See fuse_file_info structure in <fuse_common.h> for more details.
 *
 * If this request is answered with an error code of ENOSYS
 * and FUSE_CAP_NO_OPEN_SUPPORT is set in
 * `fuse_conn_info.capable`, this is treated as success and
 * future calls to open will also succeed without being send
 * to the filesystem process.
 *
 */
int nerv_open (const char *path, struct fuse_file_info *fi){
    printLog("open",path);

    struct nodeInfo* node=getNodeInfo(path);
    if(node==NULL){
        if ((fi->flags & O_ACCMODE) == O_RDONLY || !(fi->flags & O_CREAT)) return -EPERM;
        node=addNode(path);
        node->isDir=0;
    }else{
        if(node->isDir==1)
            return -EISDIR;
    }
    fi->fh=(uint64_t) node;
    return 0;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.	 An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 */
int nerv_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    printLog("read",path);

    struct nodeInfo* node=getNodeInfo(path);
    if(node==NULL)
        return -ENOENT;
    assert(node==(struct nodeInfo*)fi->fh);
    if(node->isDir==1)
        return -EISDIR;
    size_t file_size = strlen(node->ctx);
    if(offset>file_size)
        return 0;
    size_t tmp=file_size-offset;
    if(tmp>size)tmp=size;
    memcpy(buf, node->ctx+offset, tmp);
    return tmp;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
 * expected to reset the setuid and setgid bits.
 */
int nerv_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info * fi){
    printLog("write",path);

    struct nodeInfo* node=getNodeInfo(path);
    if(node==NULL)
        return -ENOENT;
    assert(node==(struct nodeInfo*)fi->fh);
    if(node->isDir==1)
        return -EISDIR;
    if(offset+size>maxFile)
        return -EPERM;
    // printf("[debug] offset %ld\n%s\n",offset,buf);
    if(isChater(path)==1){
        const char* cName=path+strlen(regFile)+1;
        struct nodeInfo* chatroomNode=getNodeInfo(chatRoomFile);
        strcat(chatroomNode->ctx,cName);
        strcat(chatroomNode->ctx,": \n");
        strcat(chatroomNode->ctx,buf);
        strcat(chatroomNode->ctx,"\n");
        strcat(node->ctx,"talk ");
        char tmp[10];
        sprintf(tmp,"%d",node->chat_times);
        strcat(node->ctx,tmp);
        strcat(node->ctx,":\n");
        strcat(node->ctx,buf);
        strcat(node->ctx,"\n\n");
        node->chat_times=node->chat_times+1;
    }else if(strcmp(path,chatRoomFile)==0){
        return -EPERM;
    }else{
        memcpy(node->ctx+offset, buf, size);
        // strcpy(node->ctx+offset, buf);
    }
    return size;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file handle.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 */
int nerv_release (const char *path, struct fuse_file_info *fi){
    printLog("release", path);

    return 0;
}

/** Read directory
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 */
int nerv_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags){
    (void) offset, (void) fi, (void) flags;
    printLog("readdir",path);

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    struct nodeInfo* dirNode=getNodeInfo(path);
    if(dirNode==NULL)
        return -ENOENT;
    if(dirNode->isDir!=1)
        return -ENOTDIR;
    getSonNode(path);
    for(int i=0;tmpNodeList[i]!=NULL;i++){
        struct nodeInfo* node=tmpNodeList[i];
        char* sonName;
        if (strcmp(path, "/") != 0) sonName=node->selfPath+strlen(dirNode->selfPath)+1;
        else sonName=node->selfPath+strlen(dirNode->selfPath);
        filler(buf,sonName,NULL,0,0);
    }
    if (strcmp(path, "/") != 0)
        for(int i=0;chater[i][0]!='\0'&&i<chaterMax;i++){
            filler(buf,chater[i],NULL,0,0);
        }
    return 0;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 */
void nerv_destroy (void *private_data){
    printLog("destroy","");
}

/**
 * Change the access and modification times of a file with
 * nanosecond resolution
 *
 * This supersedes the old utime() interface.  New applications
 * should use this.
 *
 * `fi` will always be NULL if the file is not currenlty open, but
 * may also be NULL if the file is open.
 *
 * See the utimensat(2) man page for details.
 */
int nerv_utimens (const char *path, const struct timespec tv[2],struct fuse_file_info *fi){
    (void) tv, (void) fi;
    printLog("utimens",path);
    return 0;
}

void init(){
    for(int i=0;i<pathMAX;i++){
        strcpy(pathList[i],"\0");
    }
    struct nodeInfo* root=addNode("/");
    root->isDir=1;
    _mkdir_(regFile);
    _touchFile_(chatRoomFile);
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the `private_data` field of
 * `struct fuse_context` to all file operations, and as a
 * parameter to the destroy() method. It overrides the initial
 * value provided to fuse_main() / fuse_new().
 */
void* nerv_init (struct fuse_conn_info *conn,struct fuse_config *cfg){
    printLog("init","");
    (void) conn;
    cfg->kernel_cache = 0;
    init();
    return NULL;
}

static struct fuse_operations oper = {
    .getattr    = nerv_getattr,
    .mknod      = nerv_mknod,
    .mkdir      = nerv_mkdir,
    .unlink     = nerv_unlink,
    .rmdir      = nerv_rmdir,
    .open       = nerv_open,
    .read       = nerv_read,
    .write      = nerv_write,
    .release    = nerv_release,
    .readdir    = nerv_readdir,
    .init       = nerv_init,
    .destroy    = nerv_destroy,
    .utimens    = nerv_utimens,
};

int main(int argc, char * argv[]) {
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	if (fuse_opt_parse(&args, NULL, NULL, NULL) == -1) return 1;
	ret = fuse_main(args.argc, args.argv, &oper, NULL);
	fuse_opt_free_args(&args);

	return ret;
}
