#!/bin/bash
set -e -u -x

function repair_wheel {
    wheel="$1"
    if ! auditwheel show "$wheel"; then
        echo "Skipping non-platform wheel $wheel"
    else
        auditwheel repair "$wheel" --plat "$PLAT" -w /io/wheelhouse/
    fi
}

# Install boost 1.41 if we're on RHEL 5.* x86-64 (default is 1.33) and symlink
if [[ $PLAT == "manylinux1_x86_64"  ]]; then
yum install -y boost141 boost141-devel
ln -s /usr/include/boost141/boost /usr/include
ln -s /usr/lib64/boost141 /usr/lib64/boost
fi

# Install boost 1.41 if we're on RHEL 5.* i686 (default is 1.33) and symlink
if [[ $PLAT == "manylinux1_i686" ]]; then
yum install -y boost141 boost141-devel
ln -s /usr/include/boost141/boost /usr/include
ln -s /usr/lib/boost141 /usr/lib/boost
fi

# Otherwise just install boost/boost-devel. No symlinks needed.
if [[ $PLAT == "manylinux2010_x86_64" ]]; then
yum install -y boost boost-devel
fi

# Other dep
yum install -y atlas-devel

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    # Skip Python2.7
    if [[ $PYBIN == *"cp36"* || $PYBIN == *"cp37"* || $PYBIN == *"cp38"* ]]; then
    "${PYBIN}/pip" wheel /io/ --no-deps -w wheelhouse/
    fi
done

# Bundle external shared libraries into the wheels
for whl in wheelhouse/*.whl; do
    repair_wheel "$whl"
done

# Install packages and test
for PYBIN in /opt/python/*/bin/; do
    if [[ $PYBIN == *"cp36"* || $PYBIN == *"cp37"* || $PYBIN == *"cp38"* ]]; then
    "${PYBIN}/pip" install csvtype --no-index -f /io/wheelhouse
    (cd "$HOME"; "${PYBIN}/pytest" csvtype/tests)
    fi
done
