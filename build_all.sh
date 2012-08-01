#!/bin/bash

eclipse -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -importAll . -cleanBuild libSensezilla
eclipse -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -importAll . -cleanBuild minspike_filter
eclipse -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -importAll . -cleanBuild library_builder
eclipse -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -importAll . -cleanBuild plotCSV

