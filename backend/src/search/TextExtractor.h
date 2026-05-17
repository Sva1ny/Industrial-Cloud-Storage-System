#pragma once
#include <string>

class TextExtractor {
public:
    // Unified entry: detect file type by extension and extract
    static std::string extract(const std::string &filepath, const std::string &fileExt);

    // Extract plain text from a .txt file
    static std::string extractTxt(const std::string &filepath);

    // Extract text from a PDF file using libpoppler
    static std::string extractPdf(const std::string &filepath);

    // Extract text from a .docx file using libxml2
    static std::string extractDocx(const std::string &filepath);

    // Extract text from .xlsx file (ZIP + XML)
    static std::string extractXlsx(const std::string &filepath);

    // Extract text from .xls file using libxls
    static std::string extractXls(const std::string &filepath);

    // Check if the file extension is supported
    static bool isSupported(const std::string &fileExt);
};
