#include <solution.h>
#include <liburing.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define QD 4
#define BatchSize 100

static int read_offset = 0;

enum operation{
    READ,
    WRITE
};

struct user_data{
    enum operation op;
    int offset;
    char buf[BatchSize];
    struct iovec iov;
};

static struct user_data* cur_data;

int send_read_request(struct io_uring *ring, struct user_data *data, int fd){
    memset(data->buf, 0x00, BatchSize);
    data->op = READ;
    data->iov.iov_base = data->buf;
    data->iov.iov_len = BatchSize;
    data->offset = read_offset;

    struct io_uring_sqe *sqe;
    sqe = io_uring_get_sqe(ring);
    io_uring_prep_readv(sqe, fd, &data->iov, 1, data->offset);
    io_uring_sqe_set_data(sqe, data);
    read_offset += BatchSize;
    int ret = io_uring_submit(ring);
    if(ret < 0){
        printf("submit error\n");
        return -ret;
    }
    return 0;
}

/*int send_work_request(struct io_uring *ring, struct user_data *data, int fd, enum operation op){
    struct io_uring_sqe *sqe;
    sqe = io_uring_get_sqe(ring);

    switch (op){
        case READ:
            memset(data->buf, 0x00, BatchSize);
            data->op = READ;
            data->iov.iov_base = data->buf;
            data->iov.iov_len = BatchSize;
            data->offset = read_offset;
            io_uring_prep_readv(sqe, fd, &data->iov, 1, data->offset);
            break;
        case WRITE:
            cur_data->op = WRITE;
            io_uring_prep_writev(sqe, fd, &cur_data->iov, 1, cur_data->offset);
            break;
    }
    io_uring_sqe_set_data(sqe, data);
    read_offset += BatchSize;
    int ret = io_uring_submit(ring);
    if(ret < 0){
        printf("submit error\n");
        return -ret;
    }
    return 0;
}*/

int check_work_done(struct io_uring *ring){
    struct io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(ring, &cqe);
    if(ret < 0){
        printf("wait error\n");
        return ret;
    }
    /*void* data_ptr = io_uring_cqe_get_data(cqe);
    if(data_ptr != NULL){
        cur_data = (struct user_data*)data_ptr;
    }*/
    cur_data = (struct user_data*)io_uring_cqe_get_data(cqe);
    io_uring_cqe_seen(ring, cqe);
    return cqe->res;
}


int send_write_request(struct io_uring *ring, int fd){
    cur_data->op = WRITE;
    struct io_uring_sqe *sqe;
    sqe = io_uring_get_sqe(ring);
    //printf("%d\n", read_offset);
    io_uring_prep_writev(sqe, fd, &cur_data->iov, 1, cur_data->offset);
    io_uring_sqe_set_data(sqe, cur_data);
    int ret = io_uring_submit(ring);
    if(ret < 0){
        printf("submit error\n");
        return -ret;
    }
    return 0;
}

int copy(int in_fd, int out_fd){
    struct io_uring read_ring;

    //get file_size
    struct stat in_stat;
    if (fstat(in_fd, &in_stat) == -1){
        //report error
        return errno;
    }
    off_t in_size = in_stat.st_size;
    int ret;
    ret = io_uring_queue_init(QD, &read_ring, 0);
    if(ret < 0){
        printf("init error\n");
        return -ret;
    }

    struct user_data data[QD];

    //FIRST QD READ
    for (int i = 0; i < QD; i++){
        //int ret = send_work_request(&read_ring, &data[i], in_fd, READ);
        int ret = send_read_request(&read_ring, &data[i], in_fd);
        if (ret != 0){
            return ret;
        }
    }

    int worker = QD;
    //while(read_offset < in_size){
    while(worker > 0){
        ret = check_work_done(&read_ring);
        worker--;
        if(ret < 0){
            fprintf(stderr, "check problem :%s\n", strerror(-ret));
            return ret;
        }

        if(cur_data->op == READ){
            cur_data->op = WRITE;
            cur_data->iov.iov_len = ret;
            send_write_request(&read_ring, out_fd);
            //send_work_request(&read_ring, cur_data, out_fd, WRITE);
            worker++;
        }
        else {
            if(read_offset >= in_size)
                continue;
            //send_work_request(&read_ring, cur_data, in_fd, READ);
            send_read_request(&read_ring, cur_data, in_fd);
            worker++;
        }
    }
    io_uring_queue_exit(&read_ring);
    return 0;
}