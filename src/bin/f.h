/*
   a filetype object uses .mark_.padw to store the ent
   for the FILE *
   */

FILE *lineedit(FILE *in);
FILE *statementedit(FILE *in);
object consfile(mfile *mem, /*@NULL@*/ FILE *fp);
object fileopen(mfile *mem, char *fn, char *mode); 
FILE *filefile(mfile *mem, object f);
bool filestatus(mfile *mem, object f);
long filebytesavailable(mfile *mem, object f);
void fileclose(mfile *mem, object f);
object fileread(mfile *mem, object f);
void filewrite(mfile *mem, object f, object b);

