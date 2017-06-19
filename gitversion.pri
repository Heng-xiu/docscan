# Based on
#  https://gist.github.com/grassator/11405930

# Need to call git with manually specified paths to repository
BASE_GIT_COMMAND = git --git-dir $$PWD/.git --work-tree $$PWD

# Get the HEAD's long hash, like '59e0f45efbbb5340cd4036'
GIT_COMMIT = $$system($$BASE_GIT_COMMAND rev-parse HEAD)
# Get the number of commits make in the history of the current HEAD
GIT_COMMIT_COUNT = $$system($$BASE_GIT_COMMAND rev-list HEAD --count)
isEmpty(GIT_COMMIT_COUNT) {
    GIT_COMMIT_COUNT = 0
}
GIT_COMMIT_DATE = $$system($$BASE_GIT_COMMAND log -1 --format=%ct)

# Adding C preprocessor #DEFINE so we can use it in C++ code
DEFINES += GIT_COMMIT=\\\"$$GIT_COMMIT\\\"
DEFINES += GIT_COMMIT_COUNT=\\\"$$GIT_COMMIT_COUNT\\\"
DEFINES += GIT_COMMIT_DATE=$$GIT_COMMIT_DATE
