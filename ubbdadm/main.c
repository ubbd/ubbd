#define _GNU_SOURCE
#include <getopt.h>
#include <sys/types.h>

#include "ubbd_mgmt.h"
#include "utils.h"
#include "ubbd_netlink.h"


enum ubbd_mgmt_cmd str_to_cmd(char *str)
{
	enum ubbd_mgmt_cmd cmd;

	if (!strcmp("map", str))
		cmd = UBBD_MGMT_CMD_MAP;
	else if (!strcmp("unmap", str))
		cmd = UBBD_MGMT_CMD_UNMAP;
	else
		cmd = -1;

	return cmd;
}

static enum ubbd_dev_type str_to_type(char *str)
{
	enum ubbd_dev_type type;

	if (!strcmp("file", str))
		type = UBBD_DEV_TYPE_FILE;
	else if (!strcmp("rbd", str))
		type = UBBD_DEV_TYPE_RBD;
	else
		type = -1;

	return type;
}

static struct option const long_options[] =
{
	{"command", required_argument, NULL, 'c'},
	{"type", required_argument, NULL, 't'},
	{"filepath", required_argument, NULL, 'f'},
	{"filesize", required_argument, NULL, 's'},
	{"pool", required_argument, NULL, 'p'},
	{"image", required_argument, NULL, 'i'},
	{"ubbdid", required_argument, NULL, 'u'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, NULL, 0},
};

static char *short_options = "c:t:f:p:i:u:h:s";

static void usage(int status)
{ 
	struct ubbd_nl_dev_status *tmp_status;
	LIST_HEAD(tmp_list);

	ubbd_nl_dev_list(&tmp_list);
	list_for_each_entry(tmp_status, &tmp_list, node) {
		ubbd_err("tmp_status: dev_id: %d, uio_id: %d, status: %d\n", tmp_status->dev_id, tmp_status->uio_id, tmp_status->status);
	}

	if (status != 0)
		fprintf(stderr, "Try `ubbdadm --help' for more information.\n");
	else {
		printf("\
			ubbdadm --command map --type file --filepath PATH --filesize SIZE\n\
			ubbdadm --command map --type rbd --pool POOL --image IMANGE \n\
			ubbdadm --command unmap --ubbdid ID\n");
	}
	exit(status);
}

static int do_file_map(char *filepath, uint64_t filesize)
{
	struct ubbd_mgmt_request req;
	int fd;

	req.cmd = UBBD_MGMT_CMD_MAP;
	req.u.add.info.type = UBBD_DEV_TYPE_FILE;
	strcpy(req.u.add.info.file.path, filepath);
	req.u.add.info.file.size = filesize;
	ubbd_err("size: %lu\n", filesize);
	ubbdd_request(&fd, &req);

	return 0;
}

static int do_rbd_map(char *pool, char *image)
{
	struct ubbd_mgmt_request req;
	int fd;

	req.cmd = UBBD_MGMT_CMD_MAP;
	req.u.add.info.type = UBBD_DEV_TYPE_RBD;
	strcpy(req.u.add.info.rbd.pool, pool);
	strcpy(req.u.add.info.rbd.image, image);
	ubbdd_request(&fd, &req);
	return 0;
}

static int do_unmap(int ubbdid)
{
	struct ubbd_mgmt_request req;
	int fd;

	req.cmd = UBBD_MGMT_CMD_UNMAP;
	req.u.remove.dev_id = ubbdid;
	ubbdd_request(&fd, &req);

	return 0;
}

int main(int argc, char **argv)
{
	int ch, longindex;
	enum ubbd_mgmt_cmd command;
	enum ubbd_dev_type type;
	char *filepath, *pool, *image;
	uint64_t filesize;
	int ubbdid;

	optopt = 0;
	while ((ch = getopt_long(argc, argv, short_options,
				 long_options, &longindex)) >= 0) {
		switch (ch) {
		case 'c':
			command = str_to_cmd(optarg);
			break;
		case 't':
			type = str_to_type(optarg);
			break;
		case 'f':
			filepath = optarg;
			break;
		case 's':
			filesize = atoll(optarg);
			break;
		case 'p':
			pool = optarg;
			break;
		case 'i':
			image = optarg;
			break;
		case 'u':
			ubbdid = atoi(optarg);
			ubbd_err("ubbdid: %d", ubbdid);
			break;
		case 'h':
			usage(0);
		}
	}

	if (optopt) {
		ubbd_err("unrecognized character '%c'\n", optopt);
		return -1;
	}

	if (command == UBBD_MGMT_CMD_MAP) {
		switch (type) {
		case UBBD_DEV_TYPE_FILE:
			do_file_map(filepath, filesize);
			break;
		case UBBD_DEV_TYPE_RBD:
			do_rbd_map(pool, image);
			break;
		default:
			printf("error type: %d\n", type);
			exit(-1);
		}
	} else if (command == UBBD_MGMT_CMD_UNMAP) {
		ubbd_err("unmap ubbdid: %d\n", ubbdid);
		do_unmap(ubbdid);
	} else {
		printf("error command: %d\n", command);
		exit(-1);
	}

	return 0;
}