#include "dir_crawl_helper.h"
#include "WorkQueue.h"

int
DirCrawlJob::run(void* args) {
    ThreadInfo* tinfo = (ThreadInfo*)args;
    WorkQueue* pQ = tinfo->pQ;
    char *file = NULL;

    printf("Thread[%d]: Starting crawl on directory: %s\n", tinfo->tid, _dirPath);

    DIR *dir = opendir(_dirPath);

    if (dir == NULL) {
        printf("Thread[%d]: Directory is NULL, exiting\n", tinfo->tid);
        return 0;
    }

    while ((file = next_file(_dirPath, dir)) != NULL) {
        if (file_type(file) == FILE_TYPE_DIR) {
            JobPtr j(new DirCrawlJob(file));
            printf("Thread[%d]: Enqueuing directory [%s] for crawl\n", tinfo->tid, file);
            pQ->enqueue(j, tinfo->tid);
        } else {
            printf("Thread[%d]: Found file [%s]\n", tinfo->tid, file);
            free(file);
            pQ->incFc();
        }
    }

    closedir(dir);

    return 0;
}

int
DirCrawlJob::jobType() {
    return 0;
}

int main(int argc, char const *argv[])
{

	if (argc != 2) {
		printf("Usage: %s <directory name>\n", argv[0]);
		return -1;
	}

	char *root_dir = (char *)argv[1];

	char *root = (char *)malloc(strlen(argv[1] + 1));
	strcpy(root, root_dir);

        FileCount fc;
        WorkQueue q(5, &fc);
        JobPtr j(new DirCrawlJob(root));
        q.enqueue(j, -1);

        q.waitForWorkers();

	int file_count = fc.getFileCount();

	// if (file_count < 0) {
	// 	printf("Failed to read the directory - %s\n", root_dir);
	// 	return -1;
	// }
	
	printf("Total File count in directory %s = %d\n", root_dir, file_count);
	
	return 0;
}
