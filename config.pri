VER_MAJ = 4
VER_MIN = 1
VER_PAT = 5

win32 {
    VERSION = $${VER_MAJ}.$${VER_MIN}.$$VER_PAT
}
else {
    VERSION = $$VER_MAJ\.$$VER_MIN\-$$VER_PAT
}

DEFINES += DAP_VERSION_MAJOR=\\\"$${VER_MAJ}\\\"
DEFINES += DAP_VERSION_MINOR=\\\"$${VER_MIN}\\\"

# Choose build. Can be changed by script build-deb
!defined(BRAND,var){
#  Default brand
    #BRAND = DiveVPN
    BRAND = KelvinVPN
}
