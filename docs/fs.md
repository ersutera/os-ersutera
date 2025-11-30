1. The root partition is /dev/nvme0n1p1.

2. brw-rw---- 1 root disk 259, 1 Nov 18 08:33 /dev/nvme0n1p1. This is not a regular file, it is a block device, indicated by the leading b.

3.  100+0 records in
    100+0 records out
    104857600 bytes (105 MB, 100 MiB) copied, 1.13244 s, 92.6 MB/s

4. Inodes created: 25,584

5. Free space: ~88.2%

6.  2   40755 (2)      0      0    1024 30-Nov-2025 08:53 .
    2   40755 (2)      0      0    1024 30-Nov-2025 08:53 ..
    11   40700 (2)      0      0   12288 30-Nov-2025 08:53 lost+found

7.  621bdd302478ddb872da72ea33c239f8  -
    total 4
    -rw------- 1 ersutera students  0 Nov 30 09:15 fileA
    -rw-rw-rw- 1 ersutera students  0 Nov 30 09:15 fileB
    ---------- 1 ersutera students  0 Nov 30 09:15 fileC
    -rw------- 1 ersutera students 36 Nov 30 09:15 whoami.txt

    umask 000: nothing removed - files get rw-rw-rw-
    umask 777: everything removed - files get no permissions
    umask 077: remove group/world - owner-only permissions

8. only world-read permission

9. 1099238163

10. Open the directory inode
    Read its data block(s) containing struct dirent entries
    For each entry:
        Skip unused ones (inum == 0)
        Print the name

    ----------------------------------
    |  inum=5   name="."             |
    |  inum=1   name=".."            |
    |  inum=12  name="foo"           |
    |  inum=19  name="bar.txt"       |
    ----------------------------------
