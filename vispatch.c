#include "Swchpal.h"
#include <fltenv.h>
#include <fltpnt.h>
#include <dos.h>
typedef struct visdat_t{
    char File[32];
    unsigned long len;
    unsigned long vislen;
    unsigned char *visdata;
    unsigned long leaflen;
    unsigned char *leafdata;
};

void loadvis(FILE *fp);
void freevis(void);
int OthrFix(unsigned long Offset, unsigned long Lenght);
FILE *InFile, *OutFile, *fVIS, *VISout;

visdat_t *visdat;

pakheader_t NewPak;
pakentry_t NewPakEnt[2048];
int NPcnt,numvis;

int mode = 0,cnt,usepak=0;
char FinBSP[256]="*.BSP", PinPAK[256]="PAK*.PAK", VIS[256]="VisPatch.dat", FoutBSP[256] = "", FoutPak[256] = "Pak*.Pak";
char File[256]="Pak*.Pak",CurName[38],Path[256]="",Path2[256],TempFile[256]="~vistmp.tmp";
char FilBak[256];
struct FIND *entry;
char *path;
long vispos, pakpos;

int main(int argc,char **argv){
    printf("Vis Patch v1.2 by Andy Bay (ABay@Teir.Com)\n");
    int tmp;
    if (argc>1)
        for (tmp=1;tmp<argc;tmp++){
            strlwr(argv[tmp]);
            if (argv[tmp][0]=='-' || argv[tmp][0]=='/'){
                if (argv[tmp][0]=='/') argv[tmp][0]='-';

                if (strcmp(argv[tmp],"-data")==0) {
                    argv[tmp][0]=0;
                    strcpy(VIS,argv[++tmp]);
                    argv[tmp][0]=0;
                    printf("The Vis data source is %s.\n",VIS);
                }

                if (strcmp(argv[tmp],"-dir")==0) {
                    argv[tmp][0]=0;
                    strcpy(Path,argv[++tmp]);
                    argv[tmp][0]=0;
                    printf("The pak/bsp directory is %s.\n",Path);
                }

                if (strcmp(argv[tmp],"-extract")==0) {
                    mode=1;
                    argv[tmp][0]=0;
                    printf("Extracting vis data to VisPatch.dat, auto-append.\n");
                }
                if (strcmp(argv[tmp],"-new")==0) {
                    mode = 2;
                    argv[tmp][0]=0;
                    strcpy(Path2,Path);
                    strcat(Path2,FoutPak);
                    entry = findfirst (Path2,0);
                    cnt = 0;
                   while (entry != NULL)
                   {
                      cnt++;
                      entry = findnext ();
                   }
                    sprintf(FoutPak,"%spak%i.pak",Path,cnt);
                    printf("The new pak file is called %s.\n",FoutPak);
                }


            }
            if (tmp<argc) if (strlen(argv[tmp])) strcpy(File,argv[tmp]);

        }
    //printf("mode: %i\n",mode);
    sprintf(TempFile,"%s%s",Path,"~vistmp.tmp");
    //printf("%s",TempFile);
    if (mode==0||mode == 2) {

        strcpy(Path2,Path);
        strcat(Path2,File);
        strcpy(FilBak,File);
        entry = findfirst (Path2, 0);
        while (entry != NULL){
            strcpy(File,entry->name);
            strcpy(Path2,Path);
            strcat(Path2,File);
            if(entry->attribute&_A_ARCH){
                //printf("%s",Path2);
                entry->attribute = entry->attribute - _A_ARCH;
                _dos_setfileattr(Path2,entry->attribute);
            }
            entry = findnext ();
        }
        int chk=0;
        fVIS = fopen(VIS,"rb");
        if (!fVIS) {printf("couldn't find the vis source file.\n");exit(2);}
        loadvis(fVIS);
        strcpy(Path2,Path);
        strcat(Path2,FilBak);
        OutFile = fopen(TempFile,"w+b");
        entry = findfirst (Path2, 0);
        cnt = 0;
        while (entry != NULL){
            cnt++;
            strcpy(File,entry->name);
            if(entry->attribute&_A_ARCH){
                entry = findnext ();
                continue;
            }
            strcpy(Path2,Path);
            strcat(Path2,File);
            InFile=fopen(Path2,"rb");
            if (!InFile) {printf("couldn't find the level file.\n");exit(2);}
            chk = ChooseLevel(File,0,100000);
            if(mode == 0){
                NPcnt = 0;
                fclose(OutFile);
                fclose(InFile);
                if(chk>0){
                    remove(Path2);
                    rename(TempFile,Path2);
                }
                OutFile = fopen(TempFile,"w+b");
            }
            else if(usepak == 1)
                fclose(InFile);
            else if(chk > 0){
                //printf("%i\n",chk);
                fclose(OutFile);
                fclose(InFile);
                strcpy(Path2,Path);
                strcat(Path2,File);
                strcpy(File,Path2);
                strrev(File);
                File[0] = 'k';
                File[1] = 'a';
                File[2] = 'b';
                strrev(File);
                remove(File);
                strcpy(Path2,Path);
                strcat(Path2,CurName);
                rename(Path2,File);
                rename(TempFile,Path2);
                OutFile = fopen(TempFile,"w+b");
            }
            else{
                fclose(OutFile);
                fclose(InFile);
                OutFile = fopen(TempFile,"w+b");
            }
            entry = findnext ();

        }
        fcloseall();
        //printf("%s\n",FoutPak);
        if(mode == 2 && usepak == 1){
            //printf("hi\n");
            rename(TempFile,FoutPak);
        }
        freevis();


    }
    else if (mode == 1){
        if(filesize(VIS)==-1)
            fVIS = fopen(VIS,"wb");
        else
            fVIS = fopen(VIS,"r+b");

        strcpy(Path2,Path);
        strcat(Path2,File);
        entry = findfirst (Path2, 0);
        while (entry != NULL){
            strcpy(File,entry->name);
            strcpy(Path2,Path);
            strcat(Path2,File);
            InFile=fopen(Path2,"r+b");
            //printf("hi\n");
            if (!InFile) {printf("couldn't find the level file.\n");exit(2);}
            ChooseFile(File,0,0);
            entry = findnext ();
        }
    }
    return 0;
}

char Pause()
{
  char c;
  //if (nopause) return 0;
  printf("\nPress Enter to continue...");
  while ((c = getchar()) != '\n') { }
  return c;
}

int ChooseLevel(char *FileSpec,unsigned long Offset,long length){
    int tmp=0,tmp2=0;

    //printf("Looking at file %s %i %i.\n",FileSpec,length,mode);

    if (strstr(strlwr(FileSpec),".pak")) {printf("Looking at file %s.\n",FileSpec);usepak=1;tmp=PakFix(Offset);}
    else if (length > 50000 && strstr(strlwr(FileSpec),".bsp")) {
        printf("Looking at file %s.\n",FileSpec);
        strcpy(CurName, FileSpec);
        tmp=BSPFix(Offset);
    }
    else if (mode == 0 && Offset > 0)  tmp = OthrFix(Offset, length);
    else if (mode == 2 && Offset > 0)  NPcnt--;

    //if (tmp==0)
        //printf("Did not process the file!\n");
    return tmp;
}

int PakFix(unsigned long Offset){
    int tmp;
    long ugh;
    int test;
    pakheader_t Pak;
    test = fwrite(&Pak, sizeof(pakheader_t),1,OutFile);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    fseek(InFile,Offset,SEEK_SET);
    fread(&Pak,sizeof(pakheader_t),1,InFile);
    unsigned long numentry=Pak.dirsize/64;
    pakentry_t *PakEnt=malloc(numentry*sizeof(pakentry_t));
    fseek(InFile,Offset+Pak.diroffset,SEEK_SET);
    fread((void*)PakEnt,sizeof(pakentry_t),numentry,InFile);
    for (int pakwalk=0;pakwalk<numentry;pakwalk++){
        strcpy(NewPakEnt[NPcnt].filename,PakEnt[pakwalk].filename);
        strcpy(CurName,PakEnt[pakwalk].filename);
        NewPakEnt[NPcnt].size = PakEnt[pakwalk].size;
        ChooseLevel((char *)PakEnt[pakwalk].filename,Offset+PakEnt[pakwalk].offset,PakEnt[pakwalk].size);
        NPcnt++;
    }
    free(PakEnt);
    //fseek(OutFile,0,SEEK_END);
    fflush(OutFile);
    Pak.diroffset = ftell(OutFile);
    //printf("%i %i\n",Pak.diroffset,NPcnt);
    Pak.dirsize = 64*NPcnt;
    ugh = ftell(OutFile);
    test = fwrite(&NewPakEnt,sizeof(pakentry_t),NPcnt,OutFile);
    if (test < NPcnt) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }

    fflush(OutFile);
    //chsize(fileno(OutFile),ftell(OutFile));
    fseek(OutFile,0,SEEK_SET);
    test = fwrite(&Pak, sizeof(pakheader_t),1,OutFile);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    fseek(OutFile,ugh,SEEK_SET);
    return numentry;
}
int OthrFix(unsigned long Offset, unsigned long Length){
    int test;
    int tmperr=fseek(InFile,Offset,SEEK_SET);
    NewPakEnt[NPcnt].offset = ftell(OutFile);
    NewPakEnt[NPcnt].size = Length;
    void *cpy;
    cpy = malloc(Length);
    fread( cpy, Length,1,InFile);
    test = fwrite(cpy,Length, 1, OutFile);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }

    free(cpy);
    return 1;
}
int BSPFix(unsigned long InitOFFS){

    int test;
    fflush(OutFile);
    NewPakEnt[NPcnt].offset = ftell(OutFile);
    if (NewPakEnt[NPcnt].size==0) NewPakEnt[NPcnt].size=filesize(File);

    //printf("Start: %i\n",NewPakEnt[NPcnt].offset);

    unsigned long here;
    int tmperr=fseek(InFile,InitOFFS,SEEK_SET);
    dheader_t bspheader;
    int tmp=fread(&bspheader, sizeof(dheader_t),1,InFile);
    if (tmp==0) return 0;
    printf("Version of bsp file is: %i\n",bspheader.version);
    printf("Vis info is at %i and is %i long.\n",bspheader.visilist.offset,bspheader.visilist.size);
    char *cpy;
    test = fwrite(&bspheader,sizeof(dheader_t),1,OutFile);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }


    char VisName[38];
    strcpy(VisName,CurName);
    strrev(VisName);
    strcat(VisName,"/");
    VisName[strcspn (VisName, "\\/")] = 0;
    strrev(VisName);
    int good=0;
    here = ftell(OutFile);
    bspheader.visilist.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    //("%s %s %i\n",VisName,CurName,good);
    for(tmp = 0;tmp<numvis;tmp++){
        ////("%s  ",
        if(!strcmpi(visdat[tmp].File,VisName)){
            good = 1;
            //("Name: %s Size: %i %i\n",VisName,visdat[tmp].vislen,tmp);
            fseek(OutFile,here,SEEK_SET);
            bspheader.visilist.size = visdat[tmp].vislen;
            test = fwrite(visdat[tmp].visdata,bspheader.visilist.size,1,OutFile);
            if (test == 0) {
                printf("Not enough disk space!!!  Failing.");
                fcloseall();
                remove(TempFile);
                freevis();
            }

            fflush(OutFile);
            bspheader.leaves.size   = visdat[tmp].leaflen;
            bspheader.leaves.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
            test = fwrite(visdat[tmp].leafdata,bspheader.leaves.size,1,OutFile);
            if (test == 0) {
                printf("Not enough disk space!!!  Failing.");
                fcloseall();
                remove(TempFile);
                freevis();
            }
        }
    }
    if(good == 0){
        if(usepak == 1) {
            fseek(InFile,InitOFFS, SEEK_SET);
            fseek(OutFile,NewPakEnt[NPcnt].offset,SEEK_SET);
            if(mode == 0){
                char *cpy;
                cpy = malloc(NewPakEnt[NPcnt].size);
                fread( cpy, NewPakEnt[NPcnt].size,1,InFile);
                test = fwrite(cpy,NewPakEnt[NPcnt].size, 1, OutFile);
                if (test == 0) {
                    printf("Not enough disk space!!!  Failing.");
                    fcloseall();
                    remove(TempFile);
                    freevis();
                }
                free(cpy);
                return 1;
            }
            else{
                NPcnt--;
                return 0;
            }
        }
        else
            return 0;//Individual file and it doesn't matter.
        //("not good\n");
        /*cpy = malloc(bspheader.visilist.size);
        fseek(InFile,InitOFFS+bspheader.visilist.offset, SEEK_SET);
        fread(cpy, 1,bspheader.visilist.size,InFile);
        fwrite(cpy,bspheader.visilist.size,1,OutFile);
        free(cpy);

        cpy = malloc(bspheader.leaves.size);
        fseek(InFile,InitOFFS+bspheader.leaves.offset, SEEK_SET);
        fread(cpy, 1,bspheader.leaves.size,InFile);
        bspheader.leaves.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
        fwrite(cpy,bspheader.leaves.size,1,OutFile);
        free(cpy);*/
        //("K: %i\n",ftell(OutFile));

    }

    cpy = malloc(bspheader.entities.size);
    fseek(InFile,InitOFFS+bspheader.entities.offset, SEEK_SET);
    fread(cpy, 1,bspheader.entities.size,InFile);
    bspheader.entities.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.entities.size,1,OutFile);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    free(cpy);
    //printf("A: %i %i\n",bspheader.entities.offset,ftell(OutFile));

    cpy = malloc(bspheader.planes.size);
    fseek(InFile,InitOFFS+bspheader.planes.offset, SEEK_SET);
    fread(cpy, 1,bspheader.planes.size,InFile);
    bspheader.planes.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.planes.size,1,OutFile);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    free(cpy);
    //printf("B: %i\n",ftell(OutFile));

    cpy = malloc(bspheader.miptex.size);
    fseek(InFile,InitOFFS+bspheader.miptex.offset, SEEK_SET);
    fread(cpy, 1,bspheader.miptex.size,InFile);
    bspheader.miptex.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.miptex.size,1,OutFile);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }

    //("C: %i\n",ftell(OutFile));

    cpy = malloc(bspheader.vertices.size);
    fseek(InFile,InitOFFS+bspheader.vertices.offset, SEEK_SET);
    fread(cpy, 1,bspheader.vertices.size,InFile);
    bspheader.vertices.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.vertices.size,1,OutFile);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }

    cpy = malloc(bspheader.nodes.size);
    fseek(InFile,InitOFFS+bspheader.nodes.offset, SEEK_SET);
    fread(cpy, 1,bspheader.nodes.size,InFile);
    bspheader.nodes.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.nodes.size,1,OutFile);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }

    cpy = malloc(bspheader.texinfo.size);
    fseek(InFile,InitOFFS+bspheader.texinfo.offset, SEEK_SET);
    fread(cpy, 1,bspheader.texinfo.size,InFile);
    bspheader.texinfo.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.texinfo.size,1,OutFile);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    //("G: %i\n",ftell(OutFile));

    cpy = malloc(bspheader.faces.size);
    fseek(InFile,InitOFFS+bspheader.faces.offset, SEEK_SET);
    fread(cpy, 1,bspheader.faces.size,InFile);
    bspheader.faces.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.faces.size,1,OutFile);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    //("H: %i\n",ftell(OutFile));

    cpy = malloc(bspheader.lightmaps.size);
    fseek(InFile,InitOFFS+bspheader.lightmaps.offset, SEEK_SET);
    fread(cpy, 1,bspheader.lightmaps.size,InFile);
    bspheader.lightmaps.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.lightmaps.size,1,OutFile);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    //("I: %i\n",ftell(OutFile));

    cpy = malloc(bspheader.clipnodes.size);
    fseek(InFile,InitOFFS+bspheader.clipnodes.offset, SEEK_SET);
    fread(cpy, 1,bspheader.clipnodes.size,InFile);
    bspheader.clipnodes.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.clipnodes.size,1,OutFile);
    free(cpy);
    //("J: %i\n",ftell(OutFile));


    cpy = malloc(bspheader.lface.size);
    fseek(InFile,InitOFFS+bspheader.lface.offset, SEEK_SET);
    fread(cpy, 1,bspheader.lface.size,InFile);
    bspheader.lface.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.lface.size,1,OutFile);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    //("L: %i\n",ftell(OutFile));

    cpy = malloc(bspheader.edges.size);
    fseek(InFile,InitOFFS+bspheader.edges.offset, SEEK_SET);
    fread(cpy, 1,bspheader.edges.size,InFile);
    bspheader.edges.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.edges.size,1,OutFile);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    //("M: %i\n",ftell(OutFile));

    cpy = malloc(bspheader.ledges.size);
    fseek(InFile,InitOFFS+bspheader.ledges.offset, SEEK_SET);
    fread(cpy, 1,bspheader.ledges.size,InFile);
    bspheader.ledges.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.ledges.size,1,OutFile);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    //("N: %i\n",ftell(OutFile));

    cpy = malloc(bspheader.models.size);
    fseek(InFile,InitOFFS+bspheader.models.offset, SEEK_SET);
    fread(cpy, 1,bspheader.models.size,InFile);
    bspheader.models.offset = ftell(OutFile)-NewPakEnt[NPcnt].offset;
    test = fwrite(cpy,bspheader.models.size,1,OutFile);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }

    here=ftell(OutFile);
    //("O: %i\n",here);
    fflush(OutFile);

    fseek(OutFile,NewPakEnt[NPcnt].offset, SEEK_SET);
    test = fwrite(&bspheader,sizeof(dheader_t),1,OutFile);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    fseek(OutFile,here, SEEK_SET);
    NewPakEnt[NPcnt].size = ftell(OutFile) - NewPakEnt[NPcnt].offset;

    //("End: %i\n",ftell(OutFile));

return 1;
}


int ChooseFile(char *FileSpec,unsigned long Offset,long length){
    int tmp=0,tmp2=0;
    if (length == 0 && strstr(strlwr(FileSpec),".pak")) {
        printf("Looking at file %s.\n",FileSpec);
        tmp=PakNew(Offset);
    }
    if (strstr(strlwr(FileSpec),".bsp")) {
        printf("Looking at file %s.\n",FileSpec);
        strcpy(CurName, FileSpec);
        tmp = BSPNew(Offset);
    }
    return tmp;
}

int PakNew(unsigned long Offset){
    int tmp;
    pakheader_t Pak;
    fseek(InFile,Offset,SEEK_SET);
    fread(&Pak,sizeof(pakheader_t),1,InFile);
    unsigned long numentry=Pak.dirsize/64;
    pakentry_t *PakEnt=malloc(numentry*sizeof(pakentry_t));
    fseek(InFile,Offset+Pak.diroffset,SEEK_SET);
    fread((void*)PakEnt,sizeof(pakentry_t),numentry,InFile);
    for (int pakwalk=0;pakwalk<numentry;pakwalk++){
        strcpy(NewPakEnt[NPcnt].filename,PakEnt[pakwalk].filename);
        strcpy(CurName,PakEnt[pakwalk].filename);
        NewPakEnt[NPcnt].size = PakEnt[pakwalk].size;
        ChooseFile((char *)PakEnt[pakwalk].filename,Offset+PakEnt[pakwalk].offset,PakEnt[pakwalk].size);
        NPcnt++;
    }
    free(PakEnt);
    return numentry;
}

int BSPNew(unsigned long InitOFFS){
    int test;
    unsigned long tes;
    int tmperr=fseek(InFile,InitOFFS,SEEK_SET);
    unsigned long len;
    dheader_t bspheader;
    int tmp=fread(&bspheader, sizeof(dheader_t),1,InFile);
    if (tmp==0) return 0;
    printf("Version of bsp file is:  %i\n",bspheader.version);
    printf("Vis info is at %i and is %i long\n",bspheader.visilist.offset,bspheader.visilist.size);
    printf("Leaf info is at %i and is %i long\n",bspheader.leaves.offset,bspheader.leaves.size);
    char *cpy;

    char VisName[38];
    strcpy(VisName,CurName);
    strrev(VisName);
    strcat(VisName,"/");
    VisName[strcspn (VisName, "/\\")] = 0;
    strrev(VisName);
    cpy = malloc(bspheader.visilist.size);
    fseek(InFile,InitOFFS+bspheader.visilist.offset, SEEK_SET);
    fread(cpy, 1,bspheader.visilist.size,InFile);
    len = filesize(VIS);
    //("%i\n",len);
    if(len > -1)
        fseek(fVIS,0,SEEK_END);
    test = fwrite(&VisName,1,32,fVIS);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    tes = bspheader.visilist.size+bspheader.leaves.size+8;
    test = fwrite(&tes,sizeof(long),1,fVIS);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }

    test = fwrite(&bspheader.visilist.size,sizeof(long),1,fVIS);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    test = fwrite(cpy,bspheader.visilist.size,1,fVIS);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }

    cpy = malloc(bspheader.leaves.size);
    fseek(InFile,InitOFFS+bspheader.leaves.offset, SEEK_SET);
    fread(cpy, 1,bspheader.leaves.size,InFile);
    test = fwrite(&bspheader.leaves.size,sizeof(long),1,fVIS);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }
    test = fwrite(cpy,bspheader.leaves.size,1,fVIS);
    free(cpy);
    if (test == 0) {
        printf("Not enough disk space!!!  Failing.");
        fcloseall();
        remove(TempFile);
        freevis();
    }



return 1;
}

void loadvis(FILE *fp){
    unsigned int cnt=0,tmp;
    char Name[32];
    unsigned long go;
    fseek(fp,0,SEEK_END);
    unsigned long len = ftell(fp);
    fseek(fp,0,SEEK_SET);
    while(ftell(fp) < len){
        cnt++;
        fread(Name,1,32,fp);
        fread(&go,1,sizeof(long),fp);
        fseek(fp,go,SEEK_CUR);
    }
    visdat = malloc(sizeof(visdat_t)*cnt);
    if(visdat == 0) {printf("Ack, not enough memory!");exit(1);}
    fseek(fp,0,SEEK_SET);
    for(tmp=0;tmp<cnt;tmp++){
        fread(visdat[tmp].File,1,32,fp);
        fread(&visdat[tmp].len,1,sizeof(long),fp);
        fread(&visdat[tmp].vislen,1,sizeof(long),fp);
        //printf("%i\n",  visdat[tmp].vislen);
        visdat[tmp].visdata = malloc(visdat[tmp].vislen);
        if(visdat[tmp].visdata == 0) {printf("Ack, not enough memory!");exit(2);}
        fread(visdat[tmp].visdata,1,visdat[tmp].vislen,fp);
        fread(&visdat[tmp].leaflen,1,sizeof(long),fp);
        visdat[tmp].leafdata = malloc(visdat[tmp].leaflen);
        if(visdat[tmp].leafdata == 0) {printf("Ack, not enough memory!");exit(2);}
        fread(visdat[tmp].leafdata,1,visdat[tmp].leaflen,fp);

    }
    numvis = cnt;
}

void freevis(){
    int tmp;
    for(tmp=0;tmp<numvis;tmp++){
        free(visdat[tmp].visdata);
        free(visdat[tmp].leafdata);
    }
    free(visdat);
}