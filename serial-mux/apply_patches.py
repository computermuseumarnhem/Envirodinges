from os.path import join, isfile, isdir

Import("env")

for dir in env.GetLibSourceDirs():
    LIBRARY_DIR = join(dir, 'AltSoftSerial')
    if isdir(LIBRARY_DIR):
        patchflag_file = join(LIBRARY_DIR, '.patching-done')
        if not isfile(patchflag_file):
            original_file = join(LIBRARY_DIR, 'config', 'AltSoftSerial_Boards.h')
            patch_file = '1.AltSoftSerial_Boards.h.patch'

            assert isfile(original_file) and isfile(patch_file)

            env.Execute(f'patch {original_file} {patch_file}')
            env.Execute(f'touch {patchflag_file}')
