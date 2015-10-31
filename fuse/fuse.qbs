import qbs
import qbs.FileInfo
import qbs.TextFile

Project {
QtApplication
{
    name: "fuse main"
    Depends { name: "fuse" }
    qbsSearchPaths: "."
    files: [
        "fuse-main.c",
    ]
}
}
