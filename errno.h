#pragma once

#define SUCCESS 0
#define ENOMEM -1       // out of memory
#define EINVAL -2
#define EIO -3
#define ENOENT -4        // Error no entry; When a file or dir does not exist
#define EMFILE -5       // Error too many files; Reached the limit for open files