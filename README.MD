## Restoid
	App to locate a file in a zfs snapshot and restore it
Intended to be used with sanoid/syncoid/findoid - amazing
utilities from the sanoid project: https://github.com/jimsalterjrs/sanoid

Restoid is a simple tool to search your ZFS snapshots for a file and restore it. 
You supply the full path to the file you want to restore as a parameter:

restoid /path/to/my/file

or it will prompt you for the path if just executed with no parameters.
```
$ restoid
 Enter the file or folder path: /home/mike/k
 Snapshots found:
 [0] Sun Sep 15 11:43:49 2024	0.3 KB	/home/mike/.zfs/snapshot/autosnap_2024-09-17_21:00:01_hourly/k
 Enter the unique ID of the snapshot to restore from: 0
 Do you want to restore to the original location and overwrite? (yes/no): yes
 Restoring from snapshot: /home/mike/.zfs/snapshot/autosnap_2024-09-17_21:00:01_hourly/k
 Restoring to: /home/mike/k
 Executing command: cp -R "/home/mike/.zfs/snapshot/autosnap_2024-09-17_21:00:01_hourly/k" "/home/mike/k"
 Restore complete.
```
I threw this together one morning when I was playing with findoid (amazing tool included with sanoid)

There will be bugs, there will be issues. I am a novice (at best) developer. The program calls the 
findoid and cp commands because I'm lazy and it works for me. If anyone else finds this useful I
may expand on the functionality.


