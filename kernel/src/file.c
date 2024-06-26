#include "klib.h"
#include "file.h"

#define TOTAL_FILE 128

file_t files[TOTAL_FILE];

static file_t *falloc() {
  // Lab3-1: find a file whose ref==0, init it, inc ref and return it, return NULL if none
  int i = 0;
  for(;i<TOTAL_FILE;i++){
    if(files[i].ref==0){
      files[i].ref++;
      files[i].type = TYPE_NONE;
      return &files[i];
    }
  }
  return NULL;
}

file_t *fopen(const char *path, int mode) {
  file_t *fp = falloc();
  inode_t *ip = NULL;
  if (!fp) goto bad;
  // TODO: Lab3-2, determine type according to mode
  // iopen in Lab3-2: if file exist, open and return it
  //       if file not exist and type==TYPE_NONE, return NULL
  //       if file not exist and type!=TYPE_NONE, create the file as type
  
  // you can ignore this in Lab3-1
  int open_type = 114514;
  if((mode&O_CREATE)==0)open_type = TYPE_NONE;
  else if((mode&O_DIR))open_type = TYPE_DIR;
  else open_type = TYPE_FILE;
  
  ip = iopen(path, open_type);
  if (!ip) goto bad;
  int type = itype(ip);
  //printf("ino = %d\n",ino);
  if (type == TYPE_FILE || type == TYPE_DIR) {
    // TODO: Lab3-2, if type is not DIR, go bad if mode&O_DIR
    if(type != TYPE_DIR){
    if(mode&O_DIR){
      goto bad;
    } 
    }
    // TODO: Lab3-2, if type is DIR, go bad if mode WRITE or TRUNC
    if(type == TYPE_DIR){
    if(mode&O_WRONLY||mode&O_RDWR||mode&O_TRUNC){
      goto bad;
    } 
    }
    // TODO: Lab3-2, if mode&O_TRUNC, trunc the file
    if(mode&O_TRUNC)itrunc(ip);

    fp->type = TYPE_FILE; // file_t don't and needn't distingush between file and dir
    fp->inode = ip;
    fp->offset = 0;
  } else if (type == TYPE_DEV) {
    fp->type = TYPE_DEV;
    fp->dev_op = dev_get(idevid(ip));
    iclose(ip);
    ip = NULL;
  } else assert(0);
  fp->readable = !(mode & O_WRONLY);
  fp->writable = (mode & O_WRONLY) || (mode & O_RDWR);
  return fp;
bad:
  if (fp) fclose(fp);
  if (ip) iclose(ip);
  return NULL;
}

int fread(file_t *file, void *buf, uint32_t size) {
  // Lab3-1, distribute read operation by file's type
  // remember to add offset if type is FILE (check if iread return value >= 0!)
  if (!file->readable) return -1;
  if(file->type==TYPE_FILE||file->type==TYPE_DIR){
    uint32_t off = iread(file->inode, file->offset, buf, size);
    if(off!=-1){
      file->offset+=off;
    }
    return off;
  }
  else if(file->type==TYPE_DEV){
    uint32_t off = file->dev_op->read(buf, size);
    return off;
  }
  else {
    return -1;
  }
}

int fwrite(file_t *file, const void *buf, uint32_t size) {
  // Lab3-1, distribute write operation by file's type
  // remember to add offset if type is FILE (check if iwrite return value >= 0!)
  if (!file->writable) return -1;
  if(file->type==TYPE_FILE||file->type==TYPE_DIR){
    uint32_t off = iwrite(file->inode, file->offset, buf, size);
    if(off!=-1){
      file->offset+=off;
    }
    return off;
  }
  else if(file->type==TYPE_DEV){
    uint32_t off = file->dev_op->write(buf, size);
    return off;
  }
  else {
    return -1;
  }
}

uint32_t fseek(file_t *file, uint32_t off, int whence) {
  // Lab3-1, change file's offset, do not let it cross file's size
  if (file->type == TYPE_FILE) {
    uint32_t sz = isize(file->inode);
    switch (whence)
    {
    case (SEEK_SET):{
      if(off>sz){
        file->offset = sz;
        //return sz;
      }
      else {
        file->offset = off;
        //return off;
      }
      break;
    }
    case(SEEK_CUR):{
      if(file->offset+off>sz){
        //uint32_t ret = sz - file->offset;
        file->offset = sz;
        //return ret;
      }
      else{
        assert(file->offset+off>=0);
        file->offset += off;
        //return off;
      }
      break;
    }
    case(SEEK_END):{
      //assert(off <= 0);
      assert(sz+off >= 0);
      file->offset = sz+off;
      if(file->offset>sz)file->offset = sz;
      //return off;
      break;
    }
    
    default:{
      assert(0);
      break;
    }
    }
    //printf("file->offset=%d",file->offset);
    return file->offset;
  }
  return -1;
}

file_t *fdup(file_t *file) {
  // Lab3-1, inc file's ref, then return itself
  file->ref++;
  return file;
}

void fclose(file_t *file) {
  // Lab3-1, dec file's ref, if ref==0 and it's a file, call iclose
  file->ref--;
  if(file->ref==0&&(file->type==TYPE_FILE || file->type==TYPE_DIR)){
    iclose(file->inode);
  }
}
