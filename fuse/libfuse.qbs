import qbs
import qbs.FileInfo
import qbs.TextFile

StaticLibrary
{
    name: "fuse"
    qbsSearchPaths: "."

    Depends {name: "configure"}
    Depends {name: "spectrum"}

    cpp.includePaths: [product.buildDirectory, product.sourceDirectory]

    Group {
        name: "settings dat file"
        files: "settings.dat"
        fileTags: "settings.dat"
    }

    Group {
        name: "settings perl scrip"
        files: "settings.pl"
        fileTags: "settings.pl"
    }

    Group {
        name: "settings header perl script"
        files: "settings-header.pl"
        fileTags: "settings-header.pl"
    }

    Group {
        name: "z80 perl script"
        prefix: "z80/"
        files: [
            "z80.pl",
        ]
        fileTags: "z80.pl"
    }

    Group {
        name: "z80 opcodes_base template"
        prefix: "z80/"
        files: [
            "opcodes_base.dat",
        ]
        fileTags: "opcodes_base.dat"
    }

    Group {
        name: "z80 opcodes_cb template"
        prefix: "z80/"
        files: [
            "opcodes_cb.dat",
        ]
        fileTags: "opcodes_cb.dat"
    }

    Group {
        name: "z80 opcodes_ddfd.dat template"
        prefix: "z80/"
        files: [
            "opcodes_ddfd.dat",
        ]
        fileTags: "opcodes_ddfd.dat"
    }

    Group {
        name: "z80 opcodes_ddfdcb template"
        prefix: "z80/"
        files: [
            "opcodes_ddfdcb.dat",
        ]
        fileTags: "opcodes_ddfdcb.dat"
    }

    Group {
        name: "z80 opcodes_ed template"
        prefix: "z80/"
        files: [
            "opcodes_ed.dat",
        ]
        fileTags: "opcodes_ed.dat"
    }

    Group {
        name: "fuse"
        files: [
            "compat.h",
            "display.c",
            "display.h",
            "event.c",
            "event.h",
            "fuse.c",
            "fuse.h",
            "input.c",
            "input.h",
            "keyboard.c",
            "keyboard.h",
            "loader.c",
            "loader.h",
            "machine.c",
            "machine.h",
            "memory.c",
            "memory.h",
            "mempool.c",
            "mempool.h",
            "menu.c",
            "menu.h",
            "module.c",
            "module.h",
            "movie.c",
            "movie.h",
            "movie_tables.h",
            "periph.c",
            "periph.h",
            "profile.c",
            "profile.h",
            "psg.c",
            "psg.h",
            "rectangle.c",
            "rectangle.h",
            "rzx.c",
            "rzx.h",
            "screenshot.c",
            "screenshot.h",
            "slt.c",
            "slt.h",
            "snapshot.c",
            "snapshot.h",
            "sound.c",
            "sound.h",
            "spectrum.c",
            "spectrum.h",
            "tape.c",
            "tape.h",
            "ui.c",
            "uidisplay.c",
            "uimedia.c",
            "utils.c",
            "utils.h",
        ]
    }

    Group {
        name: "z80"
        prefix: "z80/"
        files: [
            "z80.c",
            "z80.h",
            "z80_checks.h",
            "z80_macros.h",
            "z80_ops.c",
        ]
    }

    Group {
        name: "pokefinder"
        prefix: "pokefinder/"
        files: [
            "pokefinder.c",
            "pokefinder.h",
            "pokemem.c",
            "pokemem.h",
        ]
    }

    Group {
        name: "peripherals"
        prefix: "peripherals/"
        files: [
            "ay.c",
            "ay.h",
            "dck.c",
            "dck.h",
            "disk/beta.c",
            "disk/beta.h",
            "disk/crc.c",
            "disk/crc.h",
            "disk/didaktik.c",
            "disk/didaktik.h",
            "disk/disciple.c",
            "disk/disciple.h",
            "disk/disk.c",
            "disk/disk.h",
            "disk/fdd.c",
            "disk/fdd.h",
            "disk/opus.c",
            "disk/opus.h",
            "disk/plusd.c",
            "disk/plusd.h",
            "disk/upd_fdc.c",
            "disk/upd_fdc.h",
            "disk/wd_fdc.c",
            "disk/wd_fdc.h",
            "flash/am29f010.c",
            "flash/am29f010.h",
            "fuller.c",
            "fuller.h",
            "ide/divide.c",
            "ide/divide.h",
            "ide/ide.c",
            "ide/ide.h",
            "ide/simpleide.c",
            "ide/simpleide.h",
            "ide/zxatasp.c",
            "ide/zxatasp.h",
            "ide/zxcf.c",
            "ide/zxcf.h",
            "if1.c",
            "if1.h",
            "if2.c",
            "if2.h",
            "joystick.c",
            "joystick.h",
            "kempmouse.c",
            "kempmouse.h",
            "melodik.c",
            "melodik.h",
            "nic/enc28j60.c",
            "nic/enc28j60.h",
            "nic/w5100.c",
            "nic/w5100.h",
            "nic/w5100_internals.h",
            "nic/w5100_socket.c",
            "printer.c",
            "printer.h",
            "scld.c",
            "scld.h",
            "speccyboot.c",
            "speccyboot.h",
            "specdrum.c",
            "specdrum.h",
            "spectranet.c",
            "spectranet.h",
            "ula.c",
            "ula.h",
            "usource.c",
            "usource.h",
        ]
    }

    Group {
        name: "machines"
        prefix: "machines/"
        files: [
            "machines.h",
            "machines_periph.c",
            "machines_periph.h",
            "pentagon.c",
            "pentagon.h",
            "pentagon1024.c",
            "pentagon512.c",
            "scorpion.c",
            "scorpion.h",
            "spec128.c",
            "spec128.h",
            "spec16.c",
            "spec48.c",
            "spec48.h",
            "spec48_ntsc.c",
            "spec_se.c",
            "specplus2.c",
            "specplus2a.c",
            "specplus3.c",
            "specplus3.h",
            "specplus3e.c",
            "tc2048.c",
            "tc2068.c",
            "tc2068.h",
            "ts2068.c",
        ]
    }

    Group {
       name: "ui"
       prefix: "ui/"
       files: [
            "ui.h",
            "uidisplay.h",
            "uijoystick.c",
            "uijoystick.h",
            "uimedia.h",
        ]
    }

//    Group {
//       name: "sdl"
//       prefix: "ui/sdl/"
//       files: "*.c"
//    }

//    Group {
//       name: "gtk"
//       prefix: "ui/gtk/"
//       files: "*.c"
//    }
}
