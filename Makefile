#
SHELL					 		= /bin/bash
.DEFAULT_GOAL 					:= help

export BUILDKIT_VERSION 		:= 1.0.0
export LOCAL_BUILDKIT_IMAGE 	:= local/buildkit/iseg-osparc:${BUILDKIT_VERSION}
export BUILDKIT_IMAGE 			:= itisfoundation/iseg-ubuntu-buildkit:${BUILDKIT_VERSION}


WORKSPACE_ROOT                  := $(shell pwd)
export ISEG_ROOT               	:= $(WORKSPACE_ROOT)

###### BUILDKIT

.PHONY: create-build-image publish-build-image

create-build-image: ## creates build image
	docker build -t $(LOCAL_BUILDKIT_IMAGE) ./docker 

publish-build-image: ## publish build image in dockerhub
	docker tag $(LOCAL_BUILDKIT_IMAGE) $(BUILDKIT_IMAGE)
	docker push $(BUILDKIT_IMAGE)


###### DEVELOPMENT

.PHONY: build-release build-debug shell run-release run-debug clean-build debug-in-container

build-release: export BUILD_TYPE=Release
build-release: ## build iSeg 
	docker-compose -f docker/docker-compose.yml run build

build-debug: export BUILD_TYPE=Debug
build-debug: ## build iSeg with debug flags
	docker-compose -f docker/docker-compose.yml run build

shell: ## enter container
	xhost + && docker-compose -f docker/docker-compose.yml run shell

run-release: export BUILD_TYPE=Release
run-release: ## start iseg
	xhost + && docker-compose -f docker/docker-compose.yml run run

run-debug: export BUILD_TYPE=Debug
run-debug: ## start iseg in debug mode
	xhost + && docker-compose -f docker/docker-compose.yml run run

clean-build: ## clean build-volume
	docker volume rm iseg-build-volume

debug-in-container: export BUILD_TYPE=Debug
debug-in-container: ## use vs code remote container debugging
	mkdir -p $(ISEG_ROOT)/.devcontainer
	cp $(ISEG_ROOT)/docker/vscode_dot_devcontainer/devcontainer.json $(ISEG_ROOT)/.devcontainer
	$(ISEG_ROOT)/docker/vscode_dot_devcontainer/code-remote-container.sh

.PHONY: help
help: ## this colorful help
	@echo "Recipes for '$(notdir $(CURDIR))':"
	@echo ""
	@awk 'BEGIN {FS = ":.*?## "} /^[[:alpha:][:space:]_-]+:.*?## / {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)
	@echo ""