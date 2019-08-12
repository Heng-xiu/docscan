/*
 * Copyright (2017) Thomas Fischer <thomas.fischer@his.se>, senior
 * lecturer at University of Skövde, as part of the LIM-IT project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

import java.io.IOException;
import java.lang.Long;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.Date;
import java.util.List;

import org.apache.commons.text.StringEscapeUtils;

import com.qoppa.pdf.PDFException;
import com.qoppa.pdf.DocumentInfo;
import com.qoppa.pdfPreflight.PDFPreflight;
import com.qoppa.pdfPreflight.profiles.PDFA__YYYY__Verification;
import com.qoppa.pdfPreflight.results.PreflightInfo;
import com.qoppa.pdfPreflight.results.PreflightResults;
import com.qoppa.pdfPreflight.results.ResultRecord;

public class ValidatePDFA_XXXX_ {

    private static void printInformation(String xmlTag, String value) {
        if (value == null) return;
        value = value.trim();
        if (value.length() == 0) return;
        System.out.println("<" + xmlTag + ">" + StringEscapeUtils.escapeXml10(value) + "</" + xmlTag + ">");
    }

    private static void printDate(String base, Date date) {
        if (date == null) return;
        Calendar calendar = new GregorianCalendar();
        calendar.setTime(date);
        System.out.println("<date base=\"" + base + "\" year=\"" + calendar.get(Calendar.YEAR) + "\" month=\"" + (calendar.get(Calendar.MONTH) + 1) + "\" day=\"" + calendar.get(Calendar.DAY_OF_MONTH) + "\" hour=\"" + calendar.get(Calendar.HOUR) + "\" minute=\"" + calendar.get(Calendar.MINUTE) + "\" second=\"" + calendar.get(Calendar.SECOND) + "\">" + StringEscapeUtils.escapeXml10(date.toString()) + "</date>");
    }

    public static void main(String[] args) {
        if (args.length == 1) {
            final String pdfpreflightkey = args[args.length - 1];
            PDFPreflight.setKey(pdfpreflightkey);
            System.err.println("Version: " + PDFPreflight.getVersion());
            System.exit(0);
        }else if (args.length != 2) {
            System.err.println("Require two arguments: PDFPreflight-key PDF-filename");
            System.exit(1);
        }

        final String pdfpreflightkey = args[args.length - 2];
        final String filename = args[args.length - 1];

        try {
            PDFPreflight.setKey(pdfpreflightkey);
            PDFPreflight pdfPreflight = new PDFPreflight(filename, null);

            PreflightResults results = pdfPreflight.verifyDocument(new PDFA__YYYY__Verification(), null);
            if (results == null) {
                System.err.println("Failed to verify document '" + filename + "'");
                System.exit(1);
            }
            System.out.println("<qoppapdfpreflight pdfa_XXXX_=\"" + (results.isSuccessful() ? "yes" : "no") + "\">");

            final PreflightInfo pi = results.getPFInfo();
            if (pi != null) {
                System.out.println("<preflightinfo>");
                printInformation("computername",pi.getComputerName());
                printInformation("operatingsystem",pi.getOSInfo());
                printInformation("username",pi.getUserName());
                printInformation("version",pi.getVersion());
                printDate("datetime",pi.getDateTime());
                printInformation("duration-milliseconds",Long.toString(pi.getDuration()));
                System.out.println("</preflightinfo>");
            } else {
                System.err.println("PreflightInfo is invalid");
            }

            final DocumentInfo di = results.getDocumentInfo();
            if (di != null) {
                System.out.println("<documentinfo>");
                printInformation("author",di.getAuthor());
                printInformation("subject",di.getSubject());
                printInformation("title",di.getTitle());
                printInformation("producer",di.getProducer());
                printInformation("creator",di.getCreator());
                printInformation("keywords",di.getKeywords());
                printDate("creation",di.getCreationDate());
                printDate("modification",di.getModDate());
                System.out.println("</documentinfo>");
            } else {
                System.err.println("DocumentInfo is invalid");
            }

            final List<ResultRecord> resultRecords = results.getResults();
            if (resultRecords == null) {
                System.err.println("List of ResultRecord is invalid");
            } else if (resultRecords.size() > 0) {
                System.out.println("<issues count=\"" + resultRecords.size() + "\">");
                for(ResultRecord rr : resultRecords) {
                    System.out.println("<issue page=\"" + rr.getPageNumber() + "\" isfixable=\"" + (rr.isFixable() ? "yes" : "no") + "\">" + StringEscapeUtils.escapeXml10(rr.getDetail()) + "</issue>");
                }
                System.out.println("</issues>");
            }

            System.out.println("</qoppapdfpreflight>");
        } catch(Throwable t) {
            System.err.println(t);
            t.printStackTrace(System.err);
        }

    }

}
