import qbs
import qbs.FileInfo
import qbs.TextFile

StaticLibrary
{
    id: libspectrum
    name: "spectrum"
    qbsSearchPaths: '.'

    cpp.includePaths: [product.buildDirectory, product.sourceDirectory]
    Depends {name: "configure"}

    Group {
        name: "common"
        files: [
            "bzip2.c",
            "creator.c",
            "csw.c",
            "ide.c",
            "memory.c",
            "pzx_read.c",
            "snapshot.c",
            "symbol_table.c",
            "tape_block.c",
            "tzx_read.c",
            "warajevo_read.c",
            "z80em.c",
            "dck.c",
            "libspectrum.c",
            "microdrive.c",
            "rzx.c",
            "snp.c",
            "szx.c",
            "tape.c",
            "tzx_write.c",
            "wav.c",
            "zlib.c",
            "crypto.c",
            "plusd.c",
            "sna.c",
            "sp.c",
            "tap.c",
            "timings.c",
            "utilities.c",
            "z80.c",
            "zxs.c",
        ]
    }

    Group {
        name: "myglib"
        prefix: "myglib/"
        files: [
            "garray.c",
            "ghash.c",
            "gslist.c",
        ]
    }

    Group {
        name: "windows"
        condition: qbs.targetOS.contains("windows")
        files: "dll.c"
    }

    Group {
        name: 'libspectrum.h templates'
        files: ['libspectrum.h.in']
        fileTags: ['libspectrum.h.in']
    }

    Group {
        name:  'snap accessors text file'
        files: ['snap_accessors.txt']
        fileTags: ['snap_accessors.txt']
    }

    Group {
        name:  'tape accessors text file'
        files: ['tape_accessors.txt']
        fileTags: ['tape_accessors.txt']
    }

    Group {
        name: "snap accessor perl scrip"
        files: ['accessor.pl']
        fileTags: ['accessor.pl']
    }

    Group {
        name: 'tape accessors perl scrip'
        files: ['tape_accessors.pl']
        fileTags: ['tape_accessors.pl ']
    }

    Group {
        name: 'tape set perl scrip'
        files: ['tape_set.pl']
        fileTags: ['tape_set.pl']
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [product.sourceDirectory, product.buildDirectory]
    }
}
