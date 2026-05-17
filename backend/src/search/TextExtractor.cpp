#include "TextExtractor.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

// ── TXT extraction ──
std::string TextExtractor::extractTxt(const std::string &filepath)
{
    std::ifstream in(filepath, std::ios::binary);
    if (!in) return "";
    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    // Remove null bytes
    content.erase(std::remove(content.begin(), content.end(), '\0'), content.end());
    return content;
}

// ── PDF extraction via libpoppler ──
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>

std::string TextExtractor::extractPdf(const std::string &filepath)
{
    poppler::document *doc = poppler::document::load_from_file(filepath);
    if (!doc || doc->is_locked()) {
        delete doc;
        return "";
    }

    std::string text;
    for (int i = 0; i < doc->pages(); i++) {
        poppler::page *page = doc->create_page(i);
        if (page) {
            poppler::byte_array utf8 = page->text().to_utf8();
            text.append(utf8.data(), utf8.size());
            text += "\n";
            delete page;
        }
    }
    delete doc;
    return text;
}

// ── DOCX extraction via libxml2 (SAX) ──
// .docx is a ZIP containing word/document.xml
#include <cstdio>
#include <cstdlib>

std::string TextExtractor::extractDocx(const std::string &filepath)
{
    // Use unzip to extract word/document.xml, then parse text elements
    std::string cmd = "unzip -p \"" + filepath + "\" word/document.xml 2>/dev/null";
    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) return "";

    std::string xml;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp))
        xml += buf;
    pclose(fp);

    if (xml.empty()) return "";

    // Simple XML text extraction: extract content between <w:t> and </w:t>
    std::string text;
    const std::string tag_start = "<w:t";
    const std::string tag_end = "</w:t>";
    size_t pos = 0;

    while (true) {
        size_t start = xml.find(tag_start, pos);
        if (start == std::string::npos) break;
        // Skip to the end of the opening tag
        size_t gt = xml.find('>', start);
        if (gt == std::string::npos) break;
        size_t end = xml.find(tag_end, gt);
        if (end == std::string::npos) break;

        text += xml.substr(gt + 1, end - gt - 1);
        text += " ";
        pos = end + tag_end.size();
    }

    return text;
}

// ── XLSX extraction via unzip + XML ──
// .xlsx is a ZIP containing xl/sharedStrings.xml (shared strings) and/or
// xl/worksheets/sheet*.xml (inline strings)

// Helper: read an XML entry from a ZIP file via unzip -p
static std::string readZipEntry(const std::string &zipPath, const std::string &entry)
{
    std::string cmd = "unzip -p \"" + zipPath + "\" " + entry + " 2>/dev/null";
    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) return "";

    std::string content;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp))
        content += buf;
    pclose(fp);
    return content;
}

// Helper: extract text between <t> and </t> tags from XML
static std::string extractTextFromXml(const std::string &xml)
{
    std::string text;
    const std::string tag_start = "<t";
    const std::string tag_end = "</t>";
    size_t pos = 0;

    while (true) {
        size_t start = xml.find(tag_start, pos);
        if (start == std::string::npos) break;
        size_t gt = xml.find('>', start);
        if (gt == std::string::npos) break;
        size_t end = xml.find(tag_end, gt);
        if (end == std::string::npos) break;

        std::string cell_text = xml.substr(gt + 1, end - gt - 1);
        // Trim whitespace
        cell_text.erase(0, cell_text.find_first_not_of(" \t\n\r"));
        cell_text.erase(cell_text.find_last_not_of(" \t\n\r") + 1);
        if (!cell_text.empty()) {
            text += cell_text;
            text += " ";
        }
        pos = end + tag_end.size();
    }

    return text;
}

std::string TextExtractor::extractXlsx(const std::string &filepath)
{
    std::string text;

    // 1. Shared strings (traditional Excel)
    std::string shared = readZipEntry(filepath, "xl/sharedStrings.xml");
    text += extractTextFromXml(shared);

    // 2. Inline strings (e.g. from openpyxl / some online editors)
    //    These use <is><t>...</t></is> inside the worksheet XML
    for (int i = 1; i <= 10; i++) {
        std::string entry = "xl/worksheets/sheet" + std::to_string(i) + ".xml";
        std::string sheet = readZipEntry(filepath, entry);
        if (sheet.empty()) break;
        text += extractTextFromXml(sheet);
    }

    return text;
}

// ── XLS extraction via libxls ──
#include <xls.h>
using namespace xls;

std::string TextExtractor::extractXls(const std::string &filepath)
{
    xlsWorkBook *wb = xls_open(filepath.c_str(), "");
    if (!wb) return "";

    xls_parseWorkBook(wb);

    std::string text;
    for (int i = 0; i < wb->sheets.count; i++) {
        xlsWorkSheet *ws = xls_getWorkSheet(wb, i);
        if (!ws) continue;

        xls_parseWorkSheet(ws);
        for (int r = 0; r <= ws->rows.lastrow; r++) {
            for (int c = 0; c <= ws->rows.lastcol; c++) {
                xlsCell *cell = xls_cell(ws, r, c);
                if (cell && cell->str) {
                    text += (const char *)cell->str;
                    text += " ";
                }
            }
        }
        xls_close_WS(ws);
    }
    xls_close_WB(wb);

    return text;
}

// ── Unified entry ──
std::string TextExtractor::extract(const std::string &filepath, const std::string &fileExt)
{
    std::string ext = fileExt;
    if (ext.empty()) {
        size_t dot = filepath.find_last_of('.');
        if (dot != std::string::npos)
            for (size_t i = dot + 1; i < filepath.size(); i++)
                ext += tolower(filepath[i]);
    } else {
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    if (ext == "txt" || ext == "md" || ext == "csv" || ext == "log" ||
        ext == "json" || ext == "xml" || ext == "yaml" || ext == "yml")
        return extractTxt(filepath);
    else if (ext == "pdf")
        return extractPdf(filepath);
    else if (ext == "docx")
        return extractDocx(filepath);
    else if (ext == "xlsx")
        return extractXlsx(filepath);
    else if (ext == "xls")
        return extractXls(filepath);

    return "";
}

bool TextExtractor::isSupported(const std::string &fileExt)
{
    std::string ext;
    for (char c : fileExt) ext += tolower(c);
    return ext == "txt" || ext == "md" || ext == "csv" || ext == "log" ||
           ext == "json" || ext == "xml" || ext == "yaml" || ext == "yml" ||
           ext == "pdf" || ext == "docx" || ext == "xlsx" || ext == "xls";
}
