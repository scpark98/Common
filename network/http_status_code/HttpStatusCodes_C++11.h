/*! \file
 *
 * HTTP Status Codes - C++11 Variant
 *
 * https://github.com/j-ulrich/http-status-codes-cpp
 *
 * Usage:
   std::cerr << HttpStatus::reasonPhrase(reply.status);
 * 
 * \version 1.6.2
 * \author Jochen Ulrich <jochenulrich@t-online.de>
 * \copyright Licensed under Creative Commons CC0 (http://creativecommons.org/publicdomain/zero/1.0/)
 */

#ifndef HTTPSTATUSCODES_CPP11_H_
#define HTTPSTATUSCODES_CPP11_H_

#include <string>
#include <tuple>

/*! Namespace for HTTP status codes and reason phrases.
 */
namespace HttpStatus
{


/*! Enum class for the HTTP status codes.
 */
enum class Code
{
	Invalid = -1, //!< An invalid status code.

	/*####### 1xx - Informational #######*/
	/* Indicates an interim response for communicating connection status
	 * or request progress prior to completing the requested action and
	 * sending a final response.
	 */
	Continue           = 100, //!< Indicates that the initial part of a request has been received and has not yet been rejected by the server.
	SwitchingProtocols = 101, //!< Indicates that the server understands and is willing to comply with the client's request, via the Upgrade header field, for a change in the application protocol being used on this connection.
	Processing         = 102, //!< Is an interim response used to inform the client that the server has accepted the complete request, but has not yet completed it.
	EarlyHints         = 103, //!< Indicates to the client that the server is likely to send a final response with the header fields included in the informational response.

	/*####### 2xx - Successful #######*/
	/* Indicates that the client's request was successfully received,
	 * understood, and accepted.
	 */
	OK                          = 200, //!< Indicates that the request has succeeded.
	Created                     = 201, //!< Indicates that the request has been fulfilled and has resulted in one or more new resources being created.
	Accepted                    = 202, //!< Indicates that the request has been accepted for processing, but the processing has not been completed.
	NonAuthoritativeInformation = 203, //!< Indicates that the request was successful but the enclosed payload has been modified from that of the origin server's 200 (OK) response by a transforming proxy.
	NoContent                   = 204, //!< Indicates that the server has successfully fulfilled the request and that there is no additional content to send in the response payload body.
	ResetContent                = 205, //!< Indicates that the server has fulfilled the request and desires that the user agent reset the \"document view\", which caused the request to be sent, to its original state as received from the origin server.
	PartialContent              = 206, //!< Indicates that the server is successfully fulfilling a range request for the target resource by transferring one or more parts of the selected representation that correspond to the satisfiable ranges found in the requests's Range header field.
	MultiStatus                 = 207, //!< Provides status for multiple independent operations.
	AlreadyReported             = 208, //!< Used inside a DAV:propstat response element to avoid enumerating the internal members of multiple bindings to the same collection repeatedly. [RFC 5842]
	IMUsed                      = 226, //!< The server has fulfilled a GET request for the resource, and the response is a representation of the result of one or more instance-manipulations applied to the current instance.

	/*####### 3xx - Redirection #######*/
	/* Indicates that further action needs to be taken by the user agent
	 * in order to fulfill the request.
	 */
	MultipleChoices   = 300, //!< Indicates that the target resource has more than one representation, each with its own more specific identifier, and information about the alternatives is being provided so that the user (or user agent) can select a preferred representation by redirecting its request to one or more of those identifiers.
	MovedPermanently  = 301, //!< Indicates that the target resource has been assigned a new permanent URI and any future references to this resource ought to use one of the enclosed URIs.
	Found             = 302, //!< Indicates that the target resource resides temporarily under a different URI.
	SeeOther          = 303, //!< Indicates that the server is redirecting the user agent to a different resource, as indicated by a URI in the Location header field, that is intended to provide an indirect response to the original request.
	NotModified       = 304, //!< Indicates that a conditional GET request has been received and would have resulted in a 200 (OK) response if it were not for the fact that the condition has evaluated to false.
	UseProxy          = 305, //!< \deprecated \parblock Due to security concerns regarding in-band configuration of a proxy. \endparblock
	                         //!< The requested resource MUST be accessed through the proxy given by the Location field.
	TemporaryRedirect = 307, //!< Indicates that the target resource resides temporarily under a different URI and the user agent MUST NOT change the request method if it performs an automatic redirection to that URI.
	PermanentRedirect = 308, //!< The target resource has been assigned a new permanent URI and any future references to this resource outght to use one of the enclosed URIs. [...] This status code is similar to 301 Moved Permanently (Section 7.3.2 of rfc7231), except that it does not allow rewriting the request method from POST to GET.

	/*####### 4xx - Client Error #######*/
	/* Indicates that the client seems to have erred.
	 */
	BadRequest                  = 400, //!< Indicates that the server cannot or will not process the request because the received syntax is invalid, nonsensical, or exceeds some limitation on what the server is willing to process.
	Unauthorized                = 401, //!< Indicates that the request has not been applied because it lacks valid authentication credentials for the target resource.
	PaymentRequired             = 402, //!< *Reserved*
	Forbidden                   = 403, //!< Indicates that the server understood the request but refuses to authorize it.
	NotFound                    = 404, //!< Indicates that the origin server did not find a current representation for the target resource or is not willing to disclose that one exists.
	MethodNotAllowed            = 405, //!< Indicates that the method specified in the request-line is known by the origin server but not supported by the target resource.
	NotAcceptable               = 406, //!< Indicates that the target resource does not have a current representation that would be acceptable to the user agent, according to the proactive negotiation header fields received in the request, and the server is unwilling to supply a default representation.
	ProxyAuthenticationRequired = 407, //!< Is similar to 401 (Unauthorized), but indicates that the client needs to authenticate itself in order to use a proxy.
	RequestTimeout              = 408, //!< Indicates that the server did not receive a complete request message within the time that it was prepared to wait.
	Conflict                    = 409, //!< Indicates that the request could not be completed due to a conflict with the current state of the resource.
	Gone                        = 410, //!< Indicates that access to the target resource is no longer available at the origin server and that this condition is likely to be permanent.
	LengthRequired              = 411, //!< Indicates that the server refuses to accept the request without a defined Content-Length.
	PreconditionFailed          = 412, //!< Indicates that one or more preconditions given in the request header fields evaluated to false when tested on the server.
	ContentTooLarge             = 413, //!< Indicates that the server is refusing to process a request because the request payload is larger than the server is willing or able to process.
	PayloadTooLarge             = 413, //!< Alias for ContentTooLarge for backward compatibility.
	URITooLong                  = 414, //!< Indicates that the server is refusing to service the request because the request-target is longer than the server is willing to interpret.
	UnsupportedMediaType        = 415, //!< Indicates that the origin server is refusing to service the request because the payload is in a format not supported by the target resource for this method.
	RangeNotSatisfiable         = 416, //!< Indicates that none of the ranges in the request's Range header field overlap the current extent of the selected resource or that the set of ranges requested has been rejected due to invalid ranges or an excessive request of small or overlapping ranges.
	ExpectationFailed           = 417, //!< Indicates that the expectation given in the request's Expect header field could not be met by at least one of the inbound servers.
	ImATeapot                   = 418, //!< Any attempt to brew coffee with a teapot should result in the error code 418 I'm a teapot.
	MisdirectedRequest          = 421, //!< Indicates that the request was directed at a server that is unable or unwilling to produce an authoritative response for the target URI.
	UnprocessableContent        = 422, //!< Means the server understands the content type of the request entity (hence a 415(Unsupported Media Type) status code is inappropriate), and the syntax of the request entity is correct (thus a 400 (Bad Request) status code is inappropriate) but was unable to process the contained instructions.
	UnprocessableEntity         = 422, //!< Alias for UnprocessableContent for backward compatibility.
	Locked                      = 423, //!< Means the source or destination resource of a method is locked.
	FailedDependency            = 424, //!< Means that the method could not be performed on the resource because the requested action depended on another action and that action failed.
	TooEarly                    = 425, //!< Indicates that the server is unwilling to risk processing a request that might be replayed.
	UpgradeRequired             = 426, //!< Indicates that the server refuses to perform the request using the current protocol but might be willing to do so after the client upgrades to a different protocol.
	PreconditionRequired        = 428, //!< Indicates that the origin server requires the request to be conditional.
	TooManyRequests             = 429, //!< Indicates that the user has sent too many requests in a given amount of time (\"rate limiting\").
	RequestHeaderFieldsTooLarge = 431, //!< Indicates that the server is unwilling to process the request because its header fields are too large.
	UnavailableForLegalReasons  = 451, //!< This status code indicates that the server is denying access to the resource in response to a legal demand.

	/*####### 5xx - Server Error #######*/
	/* Indicates that the server is aware that it has erred
	 * or is incapable of performing the requested method.
	 */
	InternalServerError           = 500, //!< Indicates that the server encountered an unexpected condition that prevented it from fulfilling the request.
	NotImplemented                = 501, //!< Indicates that the server does not support the functionality required to fulfill the request.
	BadGateway                    = 502, //!< Indicates that the server, while acting as a gateway or proxy, received an invalid response from an inbound server it accessed while attempting to fulfill the request.
	ServiceUnavailable            = 503, //!< Indicates that the server is currently unable to handle the request due to a temporary overload or scheduled maintenance, which will likely be alleviated after some delay.
	GatewayTimeout                = 504, //!< Indicates that the server, while acting as a gateway or proxy, did not receive a timely response from an upstream server it needed to access in order to complete the request.
	HTTPVersionNotSupported       = 505, //!< Indicates that the server does not support, or refuses to support, the protocol version that was used in the request message.
	VariantAlsoNegotiates         = 506, //!< Indicates that the server has an internal configuration error: the chosen variant resource is configured to engage in transparent content negotiation itself, and is therefore not a proper end point in the negotiation process.
	InsufficientStorage           = 507, //!< Means the method could not be performed on the resource because the server is unable to store the representation needed to successfully complete the request.
	LoopDetected                  = 508, //!< Indicates that the server terminated an operation because it encountered an infinite loop while processing a request with "Depth: infinity". [RFC 5842]
	NotExtended                   = 510, //!< \deprecated \parblock Obsoleted as the experiment has ended and there is no evidence of widespread use. \endparblock
	                                     //!< The policy for accessing the resource has not been met in the request. [RFC 2774]
	NetworkAuthenticationRequired = 511, //!< Indicates that the client needs to authenticate to gain network access.

	xxx_max = 1023
};

/*! Converts a Code to its corresponding integer value.
 * \param code The code to be converted.
 * \return The numeric value of \p code.
 * \since 1.2.0
 */
inline int toInt(Code code)
{
	return static_cast<int>(code);
}

inline bool isInformational(int code) { return (code >= 100 && code < 200); } //!< \returns \c true if the given \p code is an informational code.
inline bool isSuccessful(int code)    { return (code >= 200 && code < 300); } //!< \returns \c true if the given \p code is a successful code.
inline bool isRedirection(int code)   { return (code >= 300 && code < 400); } //!< \returns \c true if the given \p code is a redirectional code.
inline bool isClientError(int code)   { return (code >= 400 && code < 500); } //!< \returns \c true if the given \p code is a client error code.
inline bool isServerError(int code)   { return (code >= 500 && code < 600); } //!< \returns \c true if the given \p code is a server error code.
inline bool isError(int code)         { return (code >= 400); }               //!< \returns \c true if the given \p code is any type of error code.

inline bool isInformational(Code code) { return isInformational(static_cast<int>(code)); } //!< \overload
inline bool isSuccessful(Code code)    { return isSuccessful(static_cast<int>(code)); }    //!< \overload
inline bool isRedirection(Code code)   { return isRedirection(static_cast<int>(code)); }   //!< \overload
inline bool isClientError(Code code)   { return isClientError(static_cast<int>(code)); }   //!< \overload
inline bool isServerError(Code code)   { return isServerError(static_cast<int>(code)); }   //!< \overload
inline bool isError(Code code)         { return isError(static_cast<int>(code)); }         //!< \overload

/*! Returns the standard HTTP reason phrase for a HTTP status code.
 * \param code An HTTP status code.
 * \return The standard HTTP reason phrase for the given \p code or an empty \c std::string()
 * if no standard phrase for the given \p code is known.
 */
//https://learn.microsoft.com/en-us/troubleshoot/developer/webapps/iis/health-diagnostic-performance/http-status-code
inline std::tuple<std::string, std::string> reasonPhrase(int code)
{
	switch (code)
	{
		//####### 1xx - Informational #######
		case 100: return { "Continue", "Initial part of the request has been received and hasn't yet been rejected by the server. The server intends to send a final response after the request has been fully received and acted upon." };
		case 101: return { "Switching Protocols", "Server understands and is willing to comply with client's request for a change in the application protocol being used." };
		case 102: return { "Processing", "A request has been received by the server, but no status was available at the time of the response." };
		case 103: return { "Early Hints", "Used to return some response headers before final HTTP message." };

		//####### 2xx - Successful #######
		case 200: return { "OK", "The request succeeded." };
		case 201: return { "Created", "The request succeeded, and a new resource was created as a result." };
		case 202: return { "Accepted", "The request has been received but not yet acted upon." };
		case 203: return { "Non-Authoritative Information", "The returned metadata is not exactly the same as is available from the origin server, but is collected from a local or a third-party copy." };
		case 204: return { "No Content", "There is no content to send for this request, but the headers are useful. The user agent may update its cached headers for this resource with the new ones." };
		case 205: return { "Reset Content", "Tells the user agent to reset the document which sent this request." };
		case 206: return { "Partial Content", "This response code is used in response to a range request when the client has requested a part or parts of a resource." };
		case 207: return { "Multi-Status", "Conveys information about multiple resources, for situations where multiple status codes might be appropriate." };
		case 208: return { "Already Reported", "Used inside a <dav:propstat> response element to avoid repeatedly enumerating the internal members of multiple bindings to the same collection." };
		case 226: return { "IM Used", "The server has fulfilled a GET request for the resource, and the response is a representation of the result of one or more instance-manipulations applied to the current instance." };

		//####### 3xx - Redirection #######
		case 300: return { "Multiple Choices", "In agent-driven content negotiation, the request has more than one possible response and the user agent or user should choose one of them." };
		case 301: return { "Moved Permanently", "The URL of the requested resource has been changed permanently. The new URL is given in the response." };
		case 302: return { "Found", "The URI of requested resource has been changed temporarily." };
		case 303: return { "See Other", "The server sent this response to direct the client to get the requested resource at another URI with a GET request." };
		case 304: return { "Not Modified", "This is used for caching purposes. It tells the client that the response has not been modified, so the client can continue to use the same cached version of the response." };
		case 305: return { "Use Proxy", "Defined in a previous version of the HTTP specification to indicate that a requested response must be accessed by a proxy. It has been deprecated due to security concerns regarding in-band configuration of a proxy." };
		case 307: return { "Temporary Redirect", "The server sends this response to direct the client to get the requested resource at another URI with the same method that was used in the prior request." };
		case 308: return { "Permanent Redirect", "This means that the resource is now permanently located at another URI, specified by the Location response header." };

		//####### 4xx - Client Error #######
		case 400: return { "Bad Request", "he request couldn't be understood by the server due to malformed syntax. The client shouldn't repeat the request without modifications. For more information, see Troubleshooting HTTP 400 Errors in IIS." };
		case 401: return { "Unauthorized", "The request hasn't been applied because it lacks valid authentication credentials for the target resource." };
		case 402: return { "Payment Required", "The initial purpose of this code was for digital payment systems, however this status code is rarely used and no standard convention exists." };
		case 403: return { "Forbidden", "The server understood the request but refuses to fulfill it." };
		case 404: return { "Not Found", "The server cannot find the requested resource. In the browser, this means the URL is not recognized." };
		case 405: return { "Method Not Allowed", "The request method is known by the server but is not supported by the target resource." };
		case 406: return { "Not Acceptable", "This response is sent when the web server, after performing server-driven content negotiation, doesn't find any content that conforms to the criteria given by the user agent." };
		case 407: return { "Proxy Authentication Required", "This is similar to 401 Unauthorized but authentication is needed to be done by a proxy." };
		case 408: return { "Request Timeout", "This response is sent on an idle connection by some servers, even without any previous request by the client." };
		case 409: return { "Conflict", "This response is sent when a request conflicts with the current state of the server." };
		case 410: return { "Gone", "" };
		case 411: return { "Length Required", "" };
		case 412: return { "Precondition Failed", "One or more conditions given in the request header fields evaluated to false when tested on the server." };
		case 413: return { "Content Too Large", "The HTTP request payload is too large." };
		case 414: return { "URI Too Long", "" };
		case 415: return { "Unsupported Media Type", "" };
		case 416: return { "Range Not Satisfiable", "" };
		case 417: return { "Expectation Failed", "" };
		case 418: return { "I'm a teapot", "" };
		case 421: return { "Misdirected Request", "" };
		case 422: return { "Unprocessable Content", "" };
		case 423: return { "Locked", "" };
		case 424: return { "Failed Dependency", "" };
		case 425: return { "Too Early", "" };
		case 426: return { "Upgrade Required", "" };
		case 428: return { "Precondition Required", "" };
		case 429: return { "Too Many Requests", "" };
		case 431: return { "Request Header Fields Too Large", "" };
		case 451: return { "Unavailable For Legal Reasons", "" };

		//####### 5xx - Server Error #######
		case 500: return { "Internal Server Error", "The server encountered an unexpected condition that prevented it from fulfilling the request." };
		case 501: return { "Not Implemented", "The server doesn't support the functionality required to fulfill the request." };
		case 502: return { "Bad Gateway", "The server, while acting as a gateway or proxy, received an invalid response from an inbound server it accessed while attempting to fulfill the request." };
		case 503: return { "Service Unavailable", "The server is currently unable to handle the request due to a temporary overload or scheduled maintenance, which will likely be alleviated after some delay." };
		case 504: return { "Gateway Timeout", "" };
		case 505: return { "HTTP Version Not Supported", "" };
		case 506: return { "Variant Also Negotiates", "" };
		case 507: return { "Insufficient Storage", "" };
		case 508: return { "Loop Detected", "" };
		case 510: return { "Not Extended", "" };
		case 511: return { "Network Authentication Required", "" };

		default: return { "Not defined status code", "" };
	}
}

/*! \overload
 *
 * \param code An HttpStatus::Code.
 * \return The standard HTTP reason phrase for the given \p code or an empty \c std::string()
 * if no standard phrase for the given \p code is known.
 */
inline std::tuple<std::string, std::string> reasonPhrase(Code code)
{
	return reasonPhrase(static_cast<int>(code));
}

} // namespace HttpStatus

#endif /* HTTPSTATUSCODES_CPP11_H_ */
