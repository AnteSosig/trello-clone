#ifndef REPO_H
#define REPO_H

#include "model.h"

// Function to add a project to the repository
int addproject(Project* project);

// Function to initialize the repository
int repo();
char* get_all_projects(void);
#endif
