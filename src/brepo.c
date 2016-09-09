/*
 * This source file is depriciated.
 * It is kept around just in case I need some of the code from in here.
 */
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>
#include <plist/plist.h>
#include "bxpkg.h"
#include "brepo.h"
#include "dxpb.h"

struct data_pack {
	char *data;
	size_t size;
};

/* Mostly taken from https://curl.haxx.se/libcurl/c/getinmemory.html */
static size_t
curl_memory_callback(void *contents, size_t size, size_t nmemb, void *userdata)
{
	struct data_pack *mem = userdata;
	size_t realsize = size * nmemb;

	if ((mem->data = realloc(mem->data, mem->size + realsize + 1)) == NULL) {
		fprintf(stderr, "Can't move more curl'd data\n");
		return 0;
	}
	memcpy(&(mem->data[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->data[mem->size] = 0;

	return realsize;
}

static plist_t
brepo_archive_to_plist(struct archive *source)
{
	char buf[80];
	struct data_pack plist = {0};
	la_ssize_t rc;
	int rc2;
	plist_t retVal;
	retVal = plist_new_dict();
	while ((rc = archive_read_data(source, buf, 80)) != 0 && rc != ARCHIVE_EOF
			&& rc != ARCHIVE_FATAL && rc != ARCHIVE_WARN) {
		rc2 = curl_memory_callback(buf, sizeof(char), rc, &plist);
		assert(rc2 == rc);
	}
	assert(strlen(plist.data) == plist.size);
	plist_from_xml(plist.data, plist.size, retVal);
	free(plist.data);
	return retVal;
}

static plist_t
brepo_repoinfo_to_plist(CURL *curl_handle, const char *repoinfo)
{
	CURLcode res;
	int rc;
	const char *tmp;
	struct data_pack info = {0};
	struct archive *source = archive_read_new();
	struct archive_entry *entry = NULL;
	plist_t retVal = NULL;

	/* curl session */
	res = curl_easy_setopt(curl_handle, CURLOPT_URL, repoinfo);
	assert(res == CURLE_OK);
	res = curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	assert(res == CURLE_OK);
	res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_memory_callback);
	assert(res == CURLE_OK);
	res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &info);
	assert(res == CURLE_OK);
	res = curl_easy_perform(curl_handle);
	assert(res == CURLE_OK);

	rc = archive_read_support_filter_gzip(source);
	assert(rc == ARCHIVE_OK);
	rc = archive_read_support_format_tar(source);
	assert(rc == ARCHIVE_OK);
	rc = archive_read_open_memory(source, info.data, info.size);
	assert(rc == ARCHIVE_OK);
	while (archive_read_next_header(source, &entry) == ARCHIVE_OK) {
		tmp = archive_entry_pathname(entry);
		assert(!strcmp(tmp, "index.plist") || !strcmp(tmp, "index-meta.plist"));
		if (strcmp(tmp, "index.plist") == 0) {
			assert(retVal == NULL);
			retVal = brepo_archive_to_plist(source);
		} else {
			archive_read_data_skip(source);
		}
	}
	rc = archive_read_free(source);
	assert(rc == ARCHIVE_OK);

	free(info.data);
	return retVal;
}

static enum pkg_archs
brepo_pkg_dict_to_arch(plist_t dict)
{
	static enum pkg_archs prediction = ARCH_NOARCH;
	char *archstr = NULL;
	plist_t tmp = plist_dict_get_item(dict, "architecture");
	plist_get_string_val(tmp, &archstr);
	if (strcmp(archstr, pkg_archs_str[prediction]) == 0) {
		return prediction;
	} /* else */
	for (enum pkg_archs i = ARCH_NOARCH; i < ARCH_HOST; i++) {
		if (i == prediction)
			continue;
		if (strcmp(archstr, pkg_archs_str[i]) == 0) {
			prediction = i;
			return i;
		}
	}
	/* If we reach here, we are out of spec */
	fprintf(stderr, "A pkg definition from repos had no architecture?");
	exit(ERR_CODE_BADDOBBY);
}

static void
brepo_merge_plists(plist_t *targets, plist_t origin)
{
	plist_t tmp, cloned;
	enum pkg_archs curarch;
	char *key;
	plist_dict_iter iter = 0;
	plist_dict_new_iter(origin, &iter);
	while (1) {
		tmp = NULL;
		plist_dict_next_item(origin, iter, &key, &tmp);
		if (tmp == NULL)
			break;
		cloned = plist_copy(tmp);
		curarch = brepo_pkg_dict_to_arch(cloned);
		plist_dict_set_item(targets[curarch], key, cloned);
		free(key);
	}
}

plist_t *
brepo_get_plists(const char **repoinfo)
{
	assert(repoinfo);
	int num_archs = ARCH_HOST-ARCH_NOARCH;
	assert(num_archs > 0);
	plist_t tmp = NULL;
	plist_t *retVal = malloc(num_archs * sizeof(plist_t));
	if (retVal == NULL) {
		perror("Out of memory for plists of archs");
		exit(ERR_CODE_NOMEM);
	}
	for (enum pkg_archs i = ARCH_NOARCH; i < ARCH_HOST; i++) {
		retVal[i] = plist_new_dict();
	}
	CURL *curl_handle;
	curl_handle = curl_easy_init();
	for (int i = 0; repoinfo[i] != NULL; i++) {
		tmp = brepo_repoinfo_to_plist(curl_handle, repoinfo[i]);
		brepo_merge_plists(retVal, tmp);
	}
	curl_easy_cleanup(curl_handle);
	return retVal;
}
