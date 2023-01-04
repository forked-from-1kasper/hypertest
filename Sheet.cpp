#include <iostream>
#include <GL/glew.h>

#include <Hyper/Sheet.hpp>
#include <PicoPNG.hpp>

Texture::Texture() : sheet(nullptr) {}

Texture::Texture(Sheet * sheet, size_t index) : sheet(sheet), index(index) {
    auto [i, j] = sheet->index(index);
    _left  = GLdouble((i + 0) * sheet->size()) / GLdouble(sheet->total());
    _right = GLdouble((i + 1) * sheet->size()) / GLdouble(sheet->total());
    _down  = GLdouble((j + 0) * sheet->size()) / GLdouble(sheet->total());
    _up    = GLdouble((j + 1) * sheet->size()) / GLdouble(sheet->total());
}

Sheet::Sheet(unsigned long size, unsigned long total) : _size(size), _total(total) {
    if (size <= 0) throw std::invalid_argument("size <= 0");
    if (total <= 0) throw std::invalid_argument("total <= 0");
    if (size > total) throw std::invalid_argument("size > total");
    if (total % size != 0) throw std::invalid_argument("`total` is not divisible by `size`");
}

Texture Sheet::attach(const std::string & file) {
    if (full()) throw std::length_error("no space left in texture sheet");
    _files.push_back(file);

    return Texture(this, files().size() - 1);
}

void Sheet::pack() {
    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, total(), total(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    for (size_t k = 0; k < files().size(); k++) {
        unsigned long width, height;
        auto [i, j] = index(k);

        auto image = PNG::load(_files[k], width, height);
        if (width != size() || height != size())
        { std::cerr << "Unexpected (" << width << " × " << height << ") texture size: " << _files[k] << std::endl; goto fin; }

        glTexSubImage2D(GL_TEXTURE_2D, 0, i * size(), j * size(), size(), size(), GL_RGBA, GL_UNSIGNED_BYTE, image.data());

        fin: image.clear();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}