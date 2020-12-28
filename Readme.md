author: Milad Bohlouli

In order to take patch from two folders use the below command:
@OS_Lab: diff -ru path/to/old path/to/new > unified.diff


In order to unpatch this change to the original folder you must use this command:
@Os_Lab: patch -p1 -d directory/of/the/original < unified.diff


my Goal of patching is to avoid uploading each project with size 
of 83 MB, so I put the original tar.xz file, and the patch file for
each project, so if you want to have the project, download the tar.xz 
and patch the .diff file on that kernel.
