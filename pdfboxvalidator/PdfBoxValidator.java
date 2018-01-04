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

import java.util.List;
import java.io.IOException;
import org.apache.pdfbox.preflight.parser.PreflightParser;
import org.apache.pdfbox.preflight.*;
import org.apache.pdfbox.preflight.exception.SyntaxValidationException;

public class PdfBoxValidator {

    public static void main(String[] args) {
        org.apache.pdfbox.preflight.ValidationResult result = null;

        if (args.length != 1 && (args.length != 2 || args[0] == "--xml")) {
            System.err.println("Require exactly one filename as command line argument or '--xml' as first argument followed by filename");
            System.exit(1);
        }
        String filename = args[args.length - 1];
        boolean xmlOutput = args[0].equals("--xml");

        try
        {
            PreflightParser parser = new PreflightParser(filename);

            /* Parse the PDF file with PreflightParser that inherits from the NonSequentialParser.
             * Some additional controls are present to check a set of PDF/A requirements.
             * (Stream length consistency, EOL after some Keyword...)
             */
            parser.parse();

            /* Once the syntax validation is done,
             * the parser can provide a PreflightDocument
             * (that inherits from PDDocument)
             * This document process the end of PDF/A validation.
             */
            PreflightDocument document = parser.getPreflightDocument();
            document.validate();

            // Get validation result
            result = document.getResult();
            document.close();

        }
        catch (SyntaxValidationException e)
        {
            /* the parse method can throw a SyntaxValidationException
             * if the PDF file can't be parsed.
             * In this case, the exception contains an instance of ValidationResult
             */
            result = e.getResult();
        }
        catch (IOException e)
        {
            System.err.println("IO error when opening " + filename);
            System.exit(1);
        }

        /// display validation result
        if (result.isValid())
        {
            if (xmlOutput)
                System.out.print("<result pdfa1b=\"yes\">");
            System.out.print("The file '" + filename + "' is a valid PDF/A-1b file");
            if (xmlOutput)
                System.out.println("</result>");
            else
                System.out.println();
            System.exit(0);
        }
        else
        {
            List<ValidationResult.ValidationError> errors = result.getErrorsList();
            if (xmlOutput)
                System.out.print("<result pdfa1b=\"no\">");
            System.out.print("The file '" + filename + "' is NOT PDF/A-1b valid, " + errors.size() + " error(s)");
            if (xmlOutput)
                System.out.println("</result>");
            else
                System.out.println();
            for (int i = 0; i < errors.size(); ++i) {
                String details = errors.get(i).getDetails();
                /// Details contains a 'buffer' that will print binary garbage; remove it
                int p = details.indexOf("; buffer");
                if (p > 0) {
                    details = details.substring(0, p + 1);
                }
                if (xmlOutput) {
                    System.out.format("<error id=\"%d\"", i);
                    if (errors.get(i).getPageNumber() != null) System.out.format(" page=\"%s\"", errors.get(i).getPageNumber());
                    System.out.format(" errorcode=\"%s\">%s</error>\n", errors.get(i).getErrorCode(), details.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"));
                } else {
                    System.out.format("%6d: %s", i, details, errors.get(i).getErrorCode());
                    if (errors.get(i).getPageNumber() != null) System.out.format(" on page %s", errors.get(i).getPageNumber());
                    System.out.format(" (error code %s)\n", errors.get(i).getErrorCode());
                }
            }
            System.exit(0);
        }
    }

}
