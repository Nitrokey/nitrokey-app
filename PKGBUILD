# Maintainer: Christoph J. Thompson <thompsonc@protonmail.ch>

pkgname=nitrokey-app
pkgver=r358.g886fed4
pkgrel=1
pkgdesc="Nitrokey management application"
arch=('i686' 'x86_64')
url="https://www.nitrokey.com"
license=('GPL3')
depends=('qt5-base' 'libusb>=1.0.0')
makedepends=('git')
source=("${pkgname}::git+https://github.com/Nitrokey/nitrokey-app"
        40-nitrokey.rules
        nitrokey-app.desktop)
install=nitrokey-app.install
sha256sums=('SKIP'
            20dff1e02ee899ebfabcbb4b995d5b0f46301969a6ee11668cd37cbef77b95ad
            6d847e94fb3e81d24765c0d0ad54f375f4943ace9b2144d5dc45e5b159184e94)

pkgver() {
  cd "${pkgname}"
  printf "r%s.g%s" \
    "$(git rev-list --count HEAD)" \
    "$(git rev-parse --short HEAD)"
}

build() {
  cd "${pkgname}"
  cmake . -DCMAKE_INSTALL_PREFIX=/usr
  make
}

package() {
  cd "${pkgname}"
  make DESTDIR="${pkgdir}" install
  install -Dm644 "${srcdir}/40-nitrokey.rules" "${pkgdir}/usr/lib/udev/rules.d/40-nitrokey.rules"
  install -Dm644 "${srcdir}/nitrokey-app.desktop" "${pkgdir}/etc/xdg/autostart/nitrokey-app.desktop"
}
