#/bin/bash

SDK_PATH=../../git_workspace/fdrsdk-xm630
FDR_PATH=./


clean()
{
    rm -rf "$FDR_PATH"/include/bashi
    rm -rf "$FDR_PATH"/include/mpp
    rm -rf "$FDR_PATH"/include/smart
    rm -rf "$FDR_PATH"/libs/*
}


copy()
{
    cp "$SDK_PATH"/include/* "$FDR_PATH"/include/ -r
    cp "$SDK_PATH"/libs/* "$FDR_PATH"/libs/ -r
}


clean
copy


