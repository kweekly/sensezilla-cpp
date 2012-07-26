#!/bin/bash

eclipse -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -importAll . -cleanBuild libSensezilla
eclipse -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -importAll . -cleanBuild fast_filter
eclipse -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -importAll . -cleanBuild library_builder

