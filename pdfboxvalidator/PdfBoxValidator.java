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
                System.out.println("<result pdfa1b=\"yes\">");
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
            for (int i = 0; i < errors.size(); ++i)
                if (xmlOutput) {
                    System.out.format("<error id=\"%d\"", i);
                    if (errors.get(i).getPageNumber() != null) System.out.format(" page=\"%s\"", errors.get(i).getPageNumber());
                    System.out.format(" errorcode=\"%s\">%s</error>\n", errors.get(i).getErrorCode(), errors.get(i).getDetails().replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"));
                } else {
                    System.out.format("%6d: %s", i, errors.get(i).getDetails(), errors.get(i).getErrorCode());
                    if (errors.get(i).getPageNumber() != null) System.out.format(" on page %s", errors.get(i).getPageNumber());
                    System.out.format(" (error code %s)\n", errors.get(i).getErrorCode());
                }
            System.exit(0);
        }
    }

}
