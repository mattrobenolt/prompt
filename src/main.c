#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "git2.h"

#define FORMAT_BASE "%s%s%s@%s%s%s %s%s%s"
#define SHORT_FORMAT FORMAT_BASE " $ \n"
#define SHORT_FORMAT_ARGS                                                      \
  style_user, user, style_reset, style_hostname, hostname, style_reset,        \
      style_wd, wd, style_reset

#define LONG_FORMAT FORMAT_BASE "%s %s%s%s $ \n"
#define LONG_FORMAT_ARGS                                                       \
  SHORT_FORMAT_ARGS, changes, style_branch, branch, style_reset

inline const char *getenv_default(const char *var) {
  const char *rv = getenv(var);
  return rv == NULL ? "" : rv;
}

void get_last_segment(const char **out, const char *s, const char c) {
  // Truncate s to everything after the last / and
  // shove back into out
  int i = 0;
  *out = s;
  while (s[i++] != '\0') {
    if (s[i] == c) {
      *out = &s[i + 1];
    }
  }
}

int main(int argc, char **argv) {
  const char *user = getenv_default("USER"),
             *style_user = getenv_default("PROMPT_STYLE_USER"),
             *style_hostname = getenv_default("PROMPT_STYLE_HOSTNAME"),
             *style_wd = getenv_default("PROMPT_STYLE_WD"),
             *style_branch = getenv_default("PROMPT_STYLE_BRANCH"),
             *style_reset = getenv_default("PROMPT_STYLE_RESET");

  char cwd[PATH_MAX], hostname[PATH_MAX];
  const char *branch, *wd, *changes = "";

  getcwd(cwd, sizeof(cwd));
  gethostname(hostname, sizeof(hostname));

  get_last_segment(&wd, cwd, '/');

  git_libgit2_init();

  git_buf gitroot = {0};
  if (git_repository_discover(&gitroot, cwd, 0, NULL) != 0) {
    printf(SHORT_FORMAT, SHORT_FORMAT_ARGS);
    return 0;
  }

  git_repository *repo;
  if (git_repository_open(&repo, gitroot.ptr) != 0) {
    printf(SHORT_FORMAT, SHORT_FORMAT_ARGS);
    return 0;
  }
  git_buf_free(&gitroot);

  git_status_list *statuses = NULL;
  git_status_options opts = GIT_STATUS_OPTIONS_INIT;
  opts.flags |= GIT_STATUS_OPT_DEFAULTS;
  git_status_list_new(&statuses, repo, &opts);

  size_t count = git_status_list_entrycount(statuses);
  for (int i = 0; i < count; ++i) {
    const git_status_entry *entry = git_status_byindex(statuses, i);
    if ((entry->status &
         (GIT_STATUS_INDEX_NEW | GIT_STATUS_INDEX_MODIFIED |
          GIT_STATUS_INDEX_DELETED | GIT_STATUS_INDEX_TYPECHANGE |
          GIT_STATUS_WT_NEW | GIT_STATUS_WT_MODIFIED | GIT_STATUS_WT_DELETED |
          GIT_STATUS_WT_RENAMED | GIT_STATUS_WT_TYPECHANGE)) > 0) {
      changes = " *";
      break;
    }
  }

  git_reference *ref = NULL;
  if (git_repository_head(&ref, repo) != 0) {
    branch = "(empty branch)";
  } else {
    const char *head = git_reference_name(ref);

    get_last_segment(&branch, head, '/');

    if (strcmp(branch, "HEAD") == 0) {
      branch = "(detached HEAD)";
    }
  }
  git_reference_free(ref);

  printf(LONG_FORMAT, LONG_FORMAT_ARGS);
  return 0;
}