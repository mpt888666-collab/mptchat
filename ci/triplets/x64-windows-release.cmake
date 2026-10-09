# 自定义 triplet：只编 Release。
# 默认的 x64-windows 会同时编 debug + release，编译时间和缓存体积都翻倍；
# CI 里只产出 Release 产物，所以单独做一个 triplet。
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_BUILD_TYPE release)
