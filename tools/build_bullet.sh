#!/usr/bin/env bash
# =====================================================================================
# Builds Bullet Physics as static libraries and copies them into this repo:
#   lib/libBulletDynamics.a, lib/libBulletCollision.a, lib/libLinearMath.a
#   include/bullet/  (headers + LICENSE.txt)
#
# The output is committed, the same way lib/libraylib.a is, so the repo builds after
# a plain clone. Run this only to upgrade or rebuild Bullet.
#
# Usage (from anywhere):
#   tools/build_bullet.sh
#
# Optional environment variables:
#   BULLET_TAG          Git tag to build                     (default: 3.25)
#   BULLET_WORK_DIR     Where to clone and build             (default: a new temp dir)
#   BULLET_OSX_SYSROOT  SDK path passed as CMAKE_OSX_SYSROOT (default: CMake's choice)
#                       Needed when the Command Line Tools SDK is newer than the linker.
#
# These flags MUST match the engine build:
#   arm64, single precision (no BT_USE_DOUBLE_PRECISION), macOS 11.0 (same as libraylib.a)
# =====================================================================================
set -euo pipefail

BULLET_TAG="${BULLET_TAG:-3.25}"
BULLET_REPO="https://github.com/bulletphysics/bullet3.git"
DEPLOYMENT_TARGET="11.0"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="${BULLET_WORK_DIR:-$(mktemp -d -t bullet3)}"
SRC_DIR="${WORK_DIR}/bullet3"
BUILD_DIR="${WORK_DIR}/build"

echo "==> Bullet ${BULLET_TAG}  ->  ${REPO_ROOT}"
echo "==> Work dir: ${WORK_DIR}"

# ------------------------------------------------------------------ Clone
if [ ! -d "${SRC_DIR}/.git" ]; then
    git clone --depth 1 --branch "${BULLET_TAG}" "${BULLET_REPO}" "${SRC_DIR}"
fi

# ------------------------------------------------------------------ Configure
SYSROOT_ARG=()
if [ -n "${BULLET_OSX_SYSROOT:-}" ]; then
    SYSROOT_ARG=(-DCMAKE_OSX_SYSROOT="${BULLET_OSX_SYSROOT}")
fi

cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET}" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DBUILD_SHARED_LIBS=OFF \
    -DUSE_DOUBLE_PRECISION=OFF \
    -DUSE_GRAPHICAL_BENCHMARK=OFF \
    -DUSE_GLUT=OFF \
    -DBUILD_BULLET2_DEMOS=OFF \
    -DBUILD_CPU_DEMOS=OFF \
    -DBUILD_OPENGL3_DEMOS=OFF \
    -DBUILD_EXTRAS=OFF \
    -DBUILD_UNIT_TESTS=OFF \
    -DBUILD_BULLET3=OFF \
    -DBUILD_PYBULLET=OFF \
    -DBUILD_ENET=OFF \
    -DBUILD_CLSOCKET=OFF \
    -DINSTALL_LIBS=OFF \
    "${SYSROOT_ARG[@]+"${SYSROOT_ARG[@]}"}"

# ------------------------------------------------------------------ Build
cmake --build "${BUILD_DIR}" --config Release -j \
    --target BulletDynamics BulletCollision LinearMath

# ------------------------------------------------------------------ Copy libraries
for LIB in BulletDynamics BulletCollision LinearMath; do
    cp "${BUILD_DIR}/src/${LIB}/lib${LIB}.a" "${REPO_ROOT}/lib/lib${LIB}.a"
done

# ------------------------------------------------------------------ Copy headers
# Only the three modules the engine uses, plus the two umbrella headers.
HEADERS_DIR="${REPO_ROOT}/include/bullet"
rm -rf "${HEADERS_DIR}"
mkdir -p "${HEADERS_DIR}"
for MODULE in BulletCollision BulletDynamics LinearMath; do
    rsync -a --prune-empty-dirs --include='*/' --include='*.h' --exclude='*' \
        "${SRC_DIR}/src/${MODULE}/" "${HEADERS_DIR}/${MODULE}/"
done
cp "${SRC_DIR}/src/btBulletCollisionCommon.h" "${SRC_DIR}/src/btBulletDynamicsCommon.h" "${HEADERS_DIR}/"
cp "${SRC_DIR}/LICENSE.txt" "${HEADERS_DIR}/LICENSE.txt"
# Not named VERSION: on case-insensitive macOS volumes it would shadow the C++ <version> header
echo "${BULLET_TAG}" > "${HEADERS_DIR}/BULLET_VERSION.txt"

echo "==> Done:"
ls -la "${REPO_ROOT}"/lib/libBullet*.a "${REPO_ROOT}"/lib/libLinearMath.a
