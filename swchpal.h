/* Switch Pal .h 1.0 */

#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

char Pause();
long Convert(unsigned long Data,long length,int dir=1);
int BSPFix(unsigned long InitOFFS);
int PakFix(unsigned long Offset);
int WadFix(unsigned long Offset);
int LmpFix(unsigned long Offset,long lenght);
int SprFix(unsigned long Offset);
int MdlFix(unsigned long Offset);

int PakNew(unsigned long Offset);
int WadNew(unsigned long Offset);
int BSPNew(unsigned long Offset);
int NewOther(unsigned long Offset,long lenght);
int ChooseFile(char *FileSpec,unsigned long Offset,long length=0);
int ChooseLevel(char *FileSpec,unsigned long Offset,long length=0);
int ChooseSource(char *FileSpec,unsigned long Offset,long length=0);

int LmpMIP(unsigned long InitOFFS);

int *Line,*Line2,*Page,*Strange;
int DWidth= 800, DHeight= 600,ScreenMode,DDepth=8;
int SetWall(int Mode);

struct MyGFX{
    long Width;
    long Height;
    long OffsX,OffsY;
    long Trans;
    unsigned char *Data;
    long filler[2];
};


#define PAK 1
#define WAD 2
#define LMP 3
#define SPR 4
#define BSP 5
#define MDL 6

typedef struct                 // A Directory entry
{ long  offset;                // Offset to entry, in bytes, from start of file
  long  size;                  // Size of entry in file, in bytes
} dentry_t;



typedef struct                 // The BSP file header
{ long  version;               // Model version, must be 0x17 (23).
  dentry_t entities;           // List of Entities.
  dentry_t planes;             // Map Planes.
                               // numplanes = size/sizeof(plane_t)
  dentry_t miptex;             // Wall Textures.
  dentry_t vertices;           // Map Vertices.
                               // numvertices = size/sizeof(vertex_t)
  dentry_t visilist;           // Leaves Visibility lists.
  dentry_t nodes;              // BSP Nodes.
                               // numnodes = size/sizeof(node_t)
  dentry_t texinfo;            // Texture Info for faces.
                               // numtexinfo = size/sizeof(texinfo_t)
  dentry_t faces;              // Faces of each surface.
                               // numfaces = size/sizeof(face_t)
  dentry_t lightmaps;          // Wall Light Maps.
  dentry_t clipnodes;          // clip nodes, for Models.
                               // numclips = size/sizeof(clipnode_t)
  dentry_t leaves;             // BSP Leaves.
                               // numlaves = size/sizeof(leaf_t)
  dentry_t lface;              // List of Faces.
  dentry_t edges;              // Edges of faces.
                               // numedges = Size/sizeof(edge_t)
  dentry_t ledges;             // List of Edges.
  dentry_t models;             // List of Models.
                               // nummodels = Size/sizeof(model_t)
} dheader_t;

typedef struct                 // Mip Texture
{ char  name[16];             // Name of the texture.
  unsigned long width;                // width of picture, must be a multiple of 8
  unsigned long height;               // height of picture, must be a multiple of 8
  unsigned long offset1;              // offset to u_char Pix[width   * height]
  unsigned long offset2;              // offset to u_char Pix[width/2 * height/2]
  unsigned long offset4;              // offset to u_char Pix[width/4 * height/4]
  unsigned long offset8;              // offset to u_char Pix[width/8 * height/8]
} miptex_t;

typedef struct
{ unsigned char magic[4];      // Pak Name of the new WAD format
  long diroffset;              // Position of WAD directory from start of file
  long dirsize;                // Number of entries * 0x40 (64 char)
} pakheader_t;

typedef struct
{ unsigned char filename[0x38];       // Name of the file, Unix style, with extension,
                               // 50 chars, padded with '\0'.
  long offset;                 // Position of the entry in PACK file
  long size;                   // Size of the entry in PACK file
} pakentry_t;

/*typedef struct
{ long width;
  long height;
  u_char Color[width*height];
} picture_t;*/

struct vec3_t {
    long xyz[3];
};
struct scalar_t {
    long Scale;
};
typedef struct
{ char id[4];                  // "IDPO" for IDPOLYGON
  long version;                // Version = 6
  vec3_t scale;                // Model scale factors.
  vec3_t origin;               // Model origin.
  scalar_t radius;             // Model bounding radius.
  vec3_t offsets;              // Eye position (useless?)
  long numskins ;              // the number of skin textures
  long skinwidth;              // Width of skin texture
                               //           must be multiple of 8
  long skinheight;             // Height of skin texture
                               //           must be multiple of 8
  long numverts;               // Number of vertices
  long numtris;                // Number of triangles surfaces
  long numframes;              // Number of frames
  long synctype;               // 0= synchron, 1= random
  long flags;                  // 0 (see Alias models)
  scalar_t size;               // average size of triangles
} mdl_t;

/*typedef struct
{ long   group;                // value = 0
  u_char skin[skinwidth*skinheight]; // the skin picture
} skin_t;


typedef struct
{ long group;                  // value = 1
  long nb;                     // number of pictures in group
  float time[nb];              // time values, for each picture
  u_char skin[nb][skinwidth*skinheight]; // the pictures
} skingroup_t;*/

typedef struct
{ char name[4];                // "IDSP"
  long ver1;                   // Version = 1
  long type;                   // See below
  float radius;                // Bounding Radius
  long maxwidth;               // Width of the largest frame
  long maxheight;              // Height of the largest frame
  long nframes;                // Number of frames
  float beamlength;            //
  long synchtype;              // 0=synchron 1=random
} spr_t;

/*typedef struct
{ long ofsx;                   // horizontal offset, in 3D space
  long ofsy;                   // vertical offset, in 3D space
  long width;                  // width of the picture
  long height;                 // height of the picture
  char Pixels[width*height];   // array of pixels (flat bitmap)
} picture;*/

typedef struct
{ unsigned char magic[4];             // "WAD2", Name of the new WAD format
  long numentries;             // Number of entries
  long diroffset;              // Position of WAD directory in file
} wadhead_t;

typedef struct
{ long offset;                 // Position of the entry in WAD
  long dsize;                  // Size of the entry in WAD file
  long size;                   // Size of the entry in memory
  char type;                   // type of entry
  char cmprs;                  // Compression. 0 if none.
  short dummy;                 // Not used
  char name[16];               // 1 to 16 characters, '\0'-padded
} wadentry_t;

struct wadoentry_t{
    long offset;               //Offset to entry
    long size;                 //Size of entry
    char name[8];              //8 letters long name null filled.

};

/*typedef struct
{ long width;                  // Picture width
  long height;                 // Picture height
  u_char Pixels[height][width]
} pichead_t;*/


typedef struct
{ short int Width;                  // Picture width
  short int Height;                 // Picture height
  short int LOffs;
  short int TOffs;
} DoomGFX_t;

struct PNames{
    FILE *Wad;
    char name[9];
    unsigned long offs;
    int EntryNum;
    int flag;
};

struct TexEnt{
    char Name[8];
    short int zero1;    //this is always 0  (we won't need this)
    short int zero2;    //this is always 0  (we won't need this)
    short int width;
    short int height;
    short int zero3;    //this is always 0  (we won't need this)
    short int zero4;    //this is always 0  (we won't need this)
    short int patches;
};

struct Patch{
    short int xoffs;
    short int yoffs;
    short int pname;
    short int one1;     //this is always 1  (we won't need it)
    short int zero5;    //this is always 0  (we won't need this)
};

struct InternalWall{
    char Name[16];
//  Rendered Flag in mipinx    int hasmipped;      //when we make it so mipping happens when we read the Wad2;
    int width;
    int height;
    int NumPat;
    Patch *patches;
};

struct mipinx{
    mipinx *Prev;
    miptex_t This;
    mipinx *Next;
    int Rendered;
    InternalWall *Wall;
};

typedef struct
{ long type;                   // Special type of leaf
  long vislist;                // Beginning of visibility lists
                               //     must be -1 or in [0,numvislist[
  //bboxshort_t bound;           // Bounding box of the leaf
  float lx,ly,lz,hx,hy,hz;
  unsigned short lface_id;            // First item of the list of faces
                               //     must be in [0,numlfaces[
  unsigned short lface_num;           // Number of faces in the leaf
  unsigned short sndwater;             // level of the four ambient sounds:
  unsigned short sndsky;               //   0    is no sound
  unsigned short sndslime;             //   0xFF is maximum volume
  unsigned short sndlava;              //
} dleaf_t;

typedef struct
{ //vec3_t   vectorS;            // S vector, horizontal in texture space)
  float ugh[2][4];
  unsigned long texture_id;         // Index of Mip Texture
                               //           must be in [0,numtex[
  unsigned long animated;           // 0 for ordinary textures, 1 for water
} surface_t;

int MakeWall(mipinx *MipSou);

void PatchWall(unsigned long Offset, unsigned char *Wall, int xoffs, int yoffs);
int Buff2Buff(int x,int y,MyGFX *Source,unsigned char *Buffer);
int CleanUp(mipinx *List);
void TexNameSwch();

