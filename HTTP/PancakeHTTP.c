#include "PancakeHTTP.h"

#ifdef PANCAKE_HTTP

#include "PancakeConfiguration.h"
#include "PancakeLogger.h"
#include "PancakeDateTime.h"

#ifdef PANCAKE_HTTP_REWRITE
#include "HTTPRewrite/PancakeHTTPRewrite.h"
#endif

PancakeModule PancakeHTTP = {
		"HTTP",
		PancakeHTTPInitialize,
		NULL,
		NULL,
		0
};

const String PancakeHTTPMethods[] = {
		StaticString("GET"),
		StaticString("POST"),
		StaticString("HEAD")
};

String PancakeHTTPAnswerCodes[] = {
		(String) {"Continue", sizeof("Continue") - 1}, /* 100 */
		(String) {"Switching Protocols", sizeof("Switching Protocols") - 1}, /* 101 */
		(String) {"", 0}, /* 102 */
		(String) {"", 0}, /* 103 */
		(String) {"", 0}, /* 104 */
		(String) {"", 0}, /* 105 */
		(String) {"", 0}, /* 106 */
		(String) {"", 0}, /* 107 */
		(String) {"", 0}, /* 108 */
		(String) {"", 0}, /* 109 */
		(String) {"", 0}, /* 110 */
		(String) {"", 0}, /* 111 */
		(String) {"", 0}, /* 112 */
		(String) {"", 0}, /* 113 */
		(String) {"", 0}, /* 114 */
		(String) {"", 0}, /* 115 */
		(String) {"", 0}, /* 116 */
		(String) {"", 0}, /* 117 */
		(String) {"", 0}, /* 118 */
		(String) {"", 0}, /* 119 */
		(String) {"", 0}, /* 120 */
		(String) {"", 0}, /* 121 */
		(String) {"", 0}, /* 122 */
		(String) {"", 0}, /* 123 */
		(String) {"", 0}, /* 124 */
		(String) {"", 0}, /* 125 */
		(String) {"", 0}, /* 126 */
		(String) {"", 0}, /* 127 */
		(String) {"", 0}, /* 128 */
		(String) {"", 0}, /* 129 */
		(String) {"", 0}, /* 130 */
		(String) {"", 0}, /* 131 */
		(String) {"", 0}, /* 132 */
		(String) {"", 0}, /* 133 */
		(String) {"", 0}, /* 134 */
		(String) {"", 0}, /* 135 */
		(String) {"", 0}, /* 136 */
		(String) {"", 0}, /* 137 */
		(String) {"", 0}, /* 138 */
		(String) {"", 0}, /* 139 */
		(String) {"", 0}, /* 140 */
		(String) {"", 0}, /* 141 */
		(String) {"", 0}, /* 142 */
		(String) {"", 0}, /* 143 */
		(String) {"", 0}, /* 144 */
		(String) {"", 0}, /* 145 */
		(String) {"", 0}, /* 146 */
		(String) {"", 0}, /* 147 */
		(String) {"", 0}, /* 148 */
		(String) {"", 0}, /* 149 */
		(String) {"", 0}, /* 150 */
		(String) {"", 0}, /* 151 */
		(String) {"", 0}, /* 152 */
		(String) {"", 0}, /* 153 */
		(String) {"", 0}, /* 154 */
		(String) {"", 0}, /* 155 */
		(String) {"", 0}, /* 156 */
		(String) {"", 0}, /* 157 */
		(String) {"", 0}, /* 158 */
		(String) {"", 0}, /* 159 */
		(String) {"", 0}, /* 160 */
		(String) {"", 0}, /* 161 */
		(String) {"", 0}, /* 162 */
		(String) {"", 0}, /* 163 */
		(String) {"", 0}, /* 164 */
		(String) {"", 0}, /* 165 */
		(String) {"", 0}, /* 166 */
		(String) {"", 0}, /* 167 */
		(String) {"", 0}, /* 168 */
		(String) {"", 0}, /* 169 */
		(String) {"", 0}, /* 170 */
		(String) {"", 0}, /* 171 */
		(String) {"", 0}, /* 172 */
		(String) {"", 0}, /* 173 */
		(String) {"", 0}, /* 174 */
		(String) {"", 0}, /* 175 */
		(String) {"", 0}, /* 176 */
		(String) {"", 0}, /* 177 */
		(String) {"", 0}, /* 178 */
		(String) {"", 0}, /* 179 */
		(String) {"", 0}, /* 180 */
		(String) {"", 0}, /* 181 */
		(String) {"", 0}, /* 182 */
		(String) {"", 0}, /* 183 */
		(String) {"", 0}, /* 184 */
		(String) {"", 0}, /* 185 */
		(String) {"", 0}, /* 186 */
		(String) {"", 0}, /* 187 */
		(String) {"", 0}, /* 188 */
		(String) {"", 0}, /* 189 */
		(String) {"", 0}, /* 190 */
		(String) {"", 0}, /* 191 */
		(String) {"", 0}, /* 192 */
		(String) {"", 0}, /* 193 */
		(String) {"", 0}, /* 194 */
		(String) {"", 0}, /* 195 */
		(String) {"", 0}, /* 196 */
		(String) {"", 0}, /* 197 */
		(String) {"", 0}, /* 198 */
		(String) {"", 0}, /* 199 */
		(String) {"OK", sizeof("OK") - 1}, /* 200 */
		(String) {"Created", sizeof("Created") - 1}, /* 201 */
		(String) {"Accepted", sizeof("Accepted") - 1}, /* 202 */
		(String) {"Non-Authoritative Information", sizeof("Non-Authoritative Information") - 1}, /* 203 */
		(String) {"No Content", sizeof("No Content") - 1}, /* 204 */
		(String) {"Reset Content", sizeof("Reset Content") - 1}, /* 205 */
		(String) {"Partial Content", sizeof("Partial Content") - 1}, /* 206 */
		(String) {"", 0}, /* 207 */
		(String) {"", 0}, /* 208 */
		(String) {"", 0}, /* 209 */
		(String) {"", 0}, /* 210 */
		(String) {"", 0}, /* 211 */
		(String) {"", 0}, /* 212 */
		(String) {"", 0}, /* 213 */
		(String) {"", 0}, /* 214 */
		(String) {"", 0}, /* 215 */
		(String) {"", 0}, /* 216 */
		(String) {"", 0}, /* 217 */
		(String) {"", 0}, /* 218 */
		(String) {"", 0}, /* 219 */
		(String) {"", 0}, /* 220 */
		(String) {"", 0}, /* 221 */
		(String) {"", 0}, /* 222 */
		(String) {"", 0}, /* 223 */
		(String) {"", 0}, /* 224 */
		(String) {"", 0}, /* 225 */
		(String) {"", 0}, /* 226 */
		(String) {"", 0}, /* 227 */
		(String) {"", 0}, /* 228 */
		(String) {"", 0}, /* 229 */
		(String) {"", 0}, /* 230 */
		(String) {"", 0}, /* 231 */
		(String) {"", 0}, /* 232 */
		(String) {"", 0}, /* 233 */
		(String) {"", 0}, /* 234 */
		(String) {"", 0}, /* 235 */
		(String) {"", 0}, /* 236 */
		(String) {"", 0}, /* 237 */
		(String) {"", 0}, /* 238 */
		(String) {"", 0}, /* 239 */
		(String) {"", 0}, /* 240 */
		(String) {"", 0}, /* 241 */
		(String) {"", 0}, /* 242 */
		(String) {"", 0}, /* 243 */
		(String) {"", 0}, /* 244 */
		(String) {"", 0}, /* 245 */
		(String) {"", 0}, /* 246 */
		(String) {"", 0}, /* 247 */
		(String) {"", 0}, /* 248 */
		(String) {"", 0}, /* 249 */
		(String) {"", 0}, /* 250 */
		(String) {"", 0}, /* 251 */
		(String) {"", 0}, /* 252 */
		(String) {"", 0}, /* 253 */
		(String) {"", 0}, /* 254 */
		(String) {"", 0}, /* 255 */
		(String) {"", 0}, /* 256 */
		(String) {"", 0}, /* 257 */
		(String) {"", 0}, /* 258 */
		(String) {"", 0}, /* 259 */
		(String) {"", 0}, /* 260 */
		(String) {"", 0}, /* 261 */
		(String) {"", 0}, /* 262 */
		(String) {"", 0}, /* 263 */
		(String) {"", 0}, /* 264 */
		(String) {"", 0}, /* 265 */
		(String) {"", 0}, /* 266 */
		(String) {"", 0}, /* 267 */
		(String) {"", 0}, /* 268 */
		(String) {"", 0}, /* 269 */
		(String) {"", 0}, /* 270 */
		(String) {"", 0}, /* 271 */
		(String) {"", 0}, /* 272 */
		(String) {"", 0}, /* 273 */
		(String) {"", 0}, /* 274 */
		(String) {"", 0}, /* 275 */
		(String) {"", 0}, /* 276 */
		(String) {"", 0}, /* 277 */
		(String) {"", 0}, /* 278 */
		(String) {"", 0}, /* 279 */
		(String) {"", 0}, /* 280 */
		(String) {"", 0}, /* 281 */
		(String) {"", 0}, /* 282 */
		(String) {"", 0}, /* 283 */
		(String) {"", 0}, /* 284 */
		(String) {"", 0}, /* 285 */
		(String) {"", 0}, /* 286 */
		(String) {"", 0}, /* 287 */
		(String) {"", 0}, /* 288 */
		(String) {"", 0}, /* 289 */
		(String) {"", 0}, /* 290 */
		(String) {"", 0}, /* 291 */
		(String) {"", 0}, /* 292 */
		(String) {"", 0}, /* 293 */
		(String) {"", 0}, /* 294 */
		(String) {"", 0}, /* 295 */
		(String) {"", 0}, /* 296 */
		(String) {"", 0}, /* 297 */
		(String) {"", 0}, /* 298 */
		(String) {"", 0}, /* 299 */
		(String) {"Multiple Choices", sizeof("Multiple Choices") - 1}, /* 300 */
		(String) {"Moved Permanently", sizeof("Moved Permanently") - 1}, /* 301 */
		(String) {"Found", sizeof("Found") - 1}, /* 302 */
		(String) {"See Other", sizeof("See Other") - 1}, /* 303 */
		(String) {"Not Modified", sizeof("Not Modified") - 1}, /* 304 */
		(String) {"Use Proxy", sizeof("Use Proxy") - 1}, /* 305 */
		(String) {"", 0}, /* 306 */
		(String) {"Temporary Redirect", sizeof("Temporary Redirect") - 1}, /* 307 */
		(String) {"", 0}, /* 308 */
		(String) {"", 0}, /* 309 */
		(String) {"", 0}, /* 310 */
		(String) {"", 0}, /* 311 */
		(String) {"", 0}, /* 312 */
		(String) {"", 0}, /* 313 */
		(String) {"", 0}, /* 314 */
		(String) {"", 0}, /* 315 */
		(String) {"", 0}, /* 316 */
		(String) {"", 0}, /* 317 */
		(String) {"", 0}, /* 318 */
		(String) {"", 0}, /* 319 */
		(String) {"", 0}, /* 320 */
		(String) {"", 0}, /* 321 */
		(String) {"", 0}, /* 322 */
		(String) {"", 0}, /* 323 */
		(String) {"", 0}, /* 324 */
		(String) {"", 0}, /* 325 */
		(String) {"", 0}, /* 326 */
		(String) {"", 0}, /* 327 */
		(String) {"", 0}, /* 328 */
		(String) {"", 0}, /* 329 */
		(String) {"", 0}, /* 330 */
		(String) {"", 0}, /* 331 */
		(String) {"", 0}, /* 332 */
		(String) {"", 0}, /* 333 */
		(String) {"", 0}, /* 334 */
		(String) {"", 0}, /* 335 */
		(String) {"", 0}, /* 336 */
		(String) {"", 0}, /* 337 */
		(String) {"", 0}, /* 338 */
		(String) {"", 0}, /* 339 */
		(String) {"", 0}, /* 340 */
		(String) {"", 0}, /* 341 */
		(String) {"", 0}, /* 342 */
		(String) {"", 0}, /* 343 */
		(String) {"", 0}, /* 344 */
		(String) {"", 0}, /* 345 */
		(String) {"", 0}, /* 346 */
		(String) {"", 0}, /* 347 */
		(String) {"", 0}, /* 348 */
		(String) {"", 0}, /* 349 */
		(String) {"", 0}, /* 350 */
		(String) {"", 0}, /* 351 */
		(String) {"", 0}, /* 352 */
		(String) {"", 0}, /* 353 */
		(String) {"", 0}, /* 354 */
		(String) {"", 0}, /* 355 */
		(String) {"", 0}, /* 356 */
		(String) {"", 0}, /* 357 */
		(String) {"", 0}, /* 358 */
		(String) {"", 0}, /* 359 */
		(String) {"", 0}, /* 360 */
		(String) {"", 0}, /* 361 */
		(String) {"", 0}, /* 362 */
		(String) {"", 0}, /* 363 */
		(String) {"", 0}, /* 364 */
		(String) {"", 0}, /* 365 */
		(String) {"", 0}, /* 366 */
		(String) {"", 0}, /* 367 */
		(String) {"", 0}, /* 368 */
		(String) {"", 0}, /* 369 */
		(String) {"", 0}, /* 370 */
		(String) {"", 0}, /* 371 */
		(String) {"", 0}, /* 372 */
		(String) {"", 0}, /* 373 */
		(String) {"", 0}, /* 374 */
		(String) {"", 0}, /* 375 */
		(String) {"", 0}, /* 376 */
		(String) {"", 0}, /* 377 */
		(String) {"", 0}, /* 378 */
		(String) {"", 0}, /* 379 */
		(String) {"", 0}, /* 380 */
		(String) {"", 0}, /* 381 */
		(String) {"", 0}, /* 382 */
		(String) {"", 0}, /* 383 */
		(String) {"", 0}, /* 384 */
		(String) {"", 0}, /* 385 */
		(String) {"", 0}, /* 386 */
		(String) {"", 0}, /* 387 */
		(String) {"", 0}, /* 388 */
		(String) {"", 0}, /* 389 */
		(String) {"", 0}, /* 390 */
		(String) {"", 0}, /* 391 */
		(String) {"", 0}, /* 392 */
		(String) {"", 0}, /* 393 */
		(String) {"", 0}, /* 394 */
		(String) {"", 0}, /* 395 */
		(String) {"", 0}, /* 396 */
		(String) {"", 0}, /* 397 */
		(String) {"", 0}, /* 398 */
		(String) {"", 0}, /* 399 */
		(String) {"Bad Request", sizeof("Bad Request") - 1}, /* 400 */
		(String) {"Unauthorized", sizeof("Unauthorized") - 1}, /* 401 */
		(String) {"Payment Required", sizeof("Payment Required") - 1}, /* 402 */
		(String) {"Forbidden", sizeof("Forbidden") - 1}, /* 403 */
		(String) {"Not Found", sizeof("Not Found") - 1}, /* 404 */
		(String) {"Method Not Allowed", sizeof("Method Not Allowed") - 1}, /* 405 */
		(String) {"Not Acceptable", sizeof("Not Acceptable") - 1}, /* 406 */
		(String) {"Proxy Authentication Required", sizeof("Proxy Authentication Required") - 1}, /* 407 */
		(String) {"Request Timeout", sizeof("Request Timeout") - 1}, /* 408 */
		(String) {"Conflict", sizeof("Conflict") - 1}, /* 409 */
		(String) {"Gone", sizeof("Gone") - 1}, /* 410 */
		(String) {"Length Required", sizeof("Length Required") - 1}, /* 411 */
		(String) {"Precondition Failed", sizeof("Precondition Failed") - 1}, /* 412 */
		(String) {"Request Entity Too Large", sizeof("Request Entity Too Large") - 1}, /* 413 */
		(String) {"Request-URI Too Long", sizeof("Request-URI Too Long") - 1}, /* 414 */
		(String) {"Unsupported Media Type", sizeof("Unsupported Media Type") - 1}, /* 415 */
		(String) {"Requested Range Not Satisfiable", sizeof("Requested Range Not Satisfiable") - 1}, /* 416 */
		(String) {"Expectation Failed", sizeof("Expectation Failed") - 1}, /* 417 */
		(String) {"", 0}, /* 418 */
		(String) {"", 0}, /* 419 */
		(String) {"", 0}, /* 420 */
		(String) {"", 0}, /* 421 */
		(String) {"", 0}, /* 422 */
		(String) {"", 0}, /* 423 */
		(String) {"", 0}, /* 424 */
		(String) {"", 0}, /* 425 */
		(String) {"", 0}, /* 426 */
		(String) {"", 0}, /* 427 */
		(String) {"", 0}, /* 428 */
		(String) {"", 0}, /* 429 */
		(String) {"", 0}, /* 430 */
		(String) {"", 0}, /* 431 */
		(String) {"", 0}, /* 432 */
		(String) {"", 0}, /* 433 */
		(String) {"", 0}, /* 434 */
		(String) {"", 0}, /* 435 */
		(String) {"", 0}, /* 436 */
		(String) {"", 0}, /* 437 */
		(String) {"", 0}, /* 438 */
		(String) {"", 0}, /* 439 */
		(String) {"", 0}, /* 440 */
		(String) {"", 0}, /* 441 */
		(String) {"", 0}, /* 442 */
		(String) {"", 0}, /* 443 */
		(String) {"", 0}, /* 444 */
		(String) {"", 0}, /* 445 */
		(String) {"", 0}, /* 446 */
		(String) {"", 0}, /* 447 */
		(String) {"", 0}, /* 448 */
		(String) {"", 0}, /* 449 */
		(String) {"", 0}, /* 450 */
		(String) {"", 0}, /* 451 */
		(String) {"", 0}, /* 452 */
		(String) {"", 0}, /* 453 */
		(String) {"", 0}, /* 454 */
		(String) {"", 0}, /* 455 */
		(String) {"", 0}, /* 456 */
		(String) {"", 0}, /* 457 */
		(String) {"", 0}, /* 458 */
		(String) {"", 0}, /* 459 */
		(String) {"", 0}, /* 460 */
		(String) {"", 0}, /* 461 */
		(String) {"", 0}, /* 462 */
		(String) {"", 0}, /* 463 */
		(String) {"", 0}, /* 464 */
		(String) {"", 0}, /* 465 */
		(String) {"", 0}, /* 466 */
		(String) {"", 0}, /* 467 */
		(String) {"", 0}, /* 468 */
		(String) {"", 0}, /* 469 */
		(String) {"", 0}, /* 470 */
		(String) {"", 0}, /* 471 */
		(String) {"", 0}, /* 472 */
		(String) {"", 0}, /* 473 */
		(String) {"", 0}, /* 474 */
		(String) {"", 0}, /* 475 */
		(String) {"", 0}, /* 476 */
		(String) {"", 0}, /* 477 */
		(String) {"", 0}, /* 478 */
		(String) {"", 0}, /* 479 */
		(String) {"", 0}, /* 480 */
		(String) {"", 0}, /* 481 */
		(String) {"", 0}, /* 482 */
		(String) {"", 0}, /* 483 */
		(String) {"", 0}, /* 484 */
		(String) {"", 0}, /* 485 */
		(String) {"", 0}, /* 486 */
		(String) {"", 0}, /* 487 */
		(String) {"", 0}, /* 488 */
		(String) {"", 0}, /* 489 */
		(String) {"", 0}, /* 490 */
		(String) {"", 0}, /* 491 */
		(String) {"", 0}, /* 492 */
		(String) {"", 0}, /* 493 */
		(String) {"", 0}, /* 494 */
		(String) {"", 0}, /* 495 */
		(String) {"", 0}, /* 496 */
		(String) {"", 0}, /* 497 */
		(String) {"", 0}, /* 498 */
		(String) {"", 0}, /* 499 */
		(String) {"Internal Server Error", sizeof("Internal Server Error") - 1}, /* 500 */
		(String) {"Not Implemented", sizeof("Not Implemented") - 1}, /* 501 */
		(String) {"Bad Gateway", sizeof("Bad Gateway") - 1}, /* 502 */
		(String) {"Service Unavailable", sizeof("Service Unavailable") - 1}, /* 503 */
		(String) {"Gateway Timeout", sizeof("Gateway Timeout") - 1}, /* 504 */
		(String) {"HTTP Version Not Supported", sizeof("HTTP Version Not Supported") - 1}, /* 505 */
		(String) {"", 0}, /* 506 */
		(String) {"", 0}, /* 507 */
		(String) {"", 0}, /* 508 */
		(String) {"", 0}, /* 509 */
		(String) {"", 0}, /* 510 */
		(String) {"", 0}, /* 511 */
		(String) {"", 0}, /* 512 */
		(String) {"", 0}, /* 513 */
		(String) {"", 0}, /* 514 */
		(String) {"", 0}, /* 515 */
		(String) {"", 0}, /* 516 */
		(String) {"", 0}, /* 517 */
		(String) {"", 0}, /* 518 */
		(String) {"", 0}, /* 519 */
		(String) {"", 0}, /* 520 */
		(String) {"", 0}, /* 521 */
		(String) {"", 0}, /* 522 */
		(String) {"", 0}, /* 523 */
		(String) {"", 0}, /* 524 */
		(String) {"", 0}, /* 525 */
		(String) {"", 0}, /* 526 */
		(String) {"", 0}, /* 527 */
		(String) {"", 0}, /* 528 */
		(String) {"", 0}, /* 529 */
		(String) {"", 0}, /* 530 */
		(String) {"", 0}, /* 531 */
		(String) {"", 0}, /* 532 */
		(String) {"", 0}, /* 533 */
		(String) {"", 0}, /* 534 */
		(String) {"", 0}, /* 535 */
		(String) {"", 0}, /* 536 */
		(String) {"", 0}, /* 537 */
		(String) {"", 0}, /* 538 */
		(String) {"", 0}, /* 539 */
		(String) {"", 0}, /* 540 */
		(String) {"", 0}, /* 541 */
		(String) {"", 0}, /* 542 */
		(String) {"", 0}, /* 543 */
		(String) {"", 0}, /* 544 */
		(String) {"", 0}, /* 545 */
		(String) {"", 0}, /* 546 */
		(String) {"", 0}, /* 547 */
		(String) {"", 0}, /* 548 */
		(String) {"", 0}, /* 549 */
		(String) {"", 0}, /* 550 */
		(String) {"", 0}, /* 551 */
		(String) {"", 0}, /* 552 */
		(String) {"", 0}, /* 553 */
		(String) {"", 0}, /* 554 */
		(String) {"", 0}, /* 555 */
		(String) {"", 0}, /* 556 */
		(String) {"", 0}, /* 557 */
		(String) {"", 0}, /* 558 */
		(String) {"", 0}, /* 559 */
		(String) {"", 0}, /* 560 */
		(String) {"", 0}, /* 561 */
		(String) {"", 0}, /* 562 */
		(String) {"", 0}, /* 563 */
		(String) {"", 0}, /* 564 */
		(String) {"", 0}, /* 565 */
		(String) {"", 0}, /* 566 */
		(String) {"", 0}, /* 567 */
		(String) {"", 0}, /* 568 */
		(String) {"", 0}, /* 569 */
		(String) {"", 0}, /* 570 */
		(String) {"", 0}, /* 571 */
		(String) {"", 0}, /* 572 */
		(String) {"", 0}, /* 573 */
		(String) {"", 0}, /* 574 */
		(String) {"", 0}, /* 575 */
		(String) {"", 0}, /* 576 */
		(String) {"", 0}, /* 577 */
		(String) {"", 0}, /* 578 */
		(String) {"", 0}, /* 579 */
		(String) {"", 0}, /* 580 */
		(String) {"", 0}, /* 581 */
		(String) {"", 0}, /* 582 */
		(String) {"", 0}, /* 583 */
		(String) {"", 0}, /* 584 */
		(String) {"", 0}, /* 585 */
		(String) {"", 0}, /* 586 */
		(String) {"", 0}, /* 587 */
		(String) {"", 0}, /* 588 */
		(String) {"", 0}, /* 589 */
		(String) {"", 0}, /* 590 */
		(String) {"", 0}, /* 591 */
		(String) {"", 0}, /* 592 */
		(String) {"", 0}, /* 593 */
		(String) {"", 0}, /* 594 */
		(String) {"", 0}, /* 595 */
		(String) {"", 0}, /* 596 */
		(String) {"", 0}, /* 597 */
		(String) {"", 0}, /* 598 */
		(String) {"", 0}, /* 599 */
};

PancakeHTTPVirtualHostIndex *PancakeHTTPVirtualHosts = NULL;
PancakeHTTPVirtualHost *PancakeHTTPDefaultVirtualHost = NULL;
PancakeHTTPConfigurationStructure PancakeHTTPConfiguration;
UInt16 PancakeHTTPNumVirtualHosts = 0;

static PancakeHTTPContentServeBackend *contentBackends = NULL;
static PancakeHTTPOutputFilter *outputFilters = NULL;
static PancakeHTTPParserHook *parserHooks = NULL;

/* Forward declarations */
static void PancakeHTTPInitializeConnection(PancakeSocket *sock);
static void PancakeHTTPReadHeaderData(PancakeSocket *sock);

PANCAKE_API void PancakeHTTPRegisterContentServeBackend(PancakeHTTPContentServeBackend *backend) {
	LL_APPEND(contentBackends, backend);
}

PANCAKE_API void PancakeHTTPRegisterOutputFilter(PancakeHTTPOutputFilter *filter) {
	LL_APPEND(outputFilters, filter);
}

PANCAKE_API void PancakeHTTPRegisterParserHook(PancakeHTTPParserHook *hook) {
	LL_APPEND(parserHooks, hook);
}

static UByte PancakeHTTPVirtualHostConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	PancakeHTTPVirtualHost *vhost;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			vhost = PancakeAllocate(sizeof(PancakeHTTPVirtualHost));
			*scope = PancakeConfigurationAddScope();
			(*scope)->data = (void*) vhost;

			vhost->configurationScope = *scope;
			vhost->contentBackends = NULL;
			vhost->numContentBackends = 0;

			vhost->outputFilters = NULL;
			vhost->numOutputFilters = 0;

			vhost->parserHooks = NULL;
			vhost->numParserHooks = 0;

#ifdef PANCAKE_HTTP_REWRITE
			vhost->rewriteConfiguration = NULL;
#endif

			PancakeHTTPNumVirtualHosts++;

			setting->hook = (void*) vhost;
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			vhost = (PancakeHTTPVirtualHost*) setting->hook;

			PancakeConfigurationDestroyScope(vhost->configurationScope);
			PancakeFree(vhost);
		} break;
	}

	return 1;
}

static UByte PancakeHTTPHostsConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			PancakeHTTPVirtualHost *vHost = (PancakeHTTPVirtualHost*) (*scope)->data;
			config_setting_t *element;
			UInt16 i = 0;

			while(element = config_setting_get_elem(setting, i++)) {
				PancakeHTTPVirtualHostIndex *index = PancakeAllocate(sizeof(PancakeHTTPVirtualHostIndex));

				index->vHost = vHost;

				HASH_ADD_KEYPTR(hh, PancakeHTTPVirtualHosts, element->value.sval, strlen(element->value.sval), index);
			}
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			config_setting_t *element;
			UInt16 i = 0;

			while(element = config_setting_get_elem(setting, i++)) {
				PancakeHTTPVirtualHostIndex *index;

				HASH_FIND(hh, PancakeHTTPVirtualHosts, element->value.sval, strlen(element->value.sval), index);
				PancakeAssert(index != NULL);

				HASH_DEL(PancakeHTTPVirtualHosts, index);
				PancakeFree(index);
			}
		} break;
	}

	return 1;
}

static UByte PancakeHTTPDefaultConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(step == PANCAKE_CONFIGURATION_INIT && setting->value.ival == 1) {
		if(PancakeHTTPDefaultVirtualHost) {
			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Another virtual host has already been set as default");
			return 0;
		}

		PancakeHTTPDefaultVirtualHost = (PancakeHTTPVirtualHost*) (*scope)->data;
	}

	return 1;
}

static UByte PancakeHTTPDocumentRootConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	String *documentRoot;

	switch(step) {
		case PANCAKE_CONFIGURATION_INIT: {
			struct stat buf;

			if(stat(setting->value.sval, &buf) == -1) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't stat document root %s: %s", setting->value.sval, strerror(errno));
				return 0;
			}

			if(!S_ISDIR(buf.st_mode)) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Document root %s is not a directory", setting->value.sval);
				return 0;
			}

#ifdef PANCAKE_HTTP_REWRITE
			PancakeHTTPRewriteConfigurationHook(step, setting, scope);
#endif

			// Make String out of document root
			documentRoot = PancakeAllocate(sizeof(String));
			documentRoot->length = strlen(setting->value.sval);
			documentRoot->value = PancakeAllocate(documentRoot->length + 1);
			memcpy(documentRoot->value, setting->value.sval, documentRoot->length + 1);

			free(setting->value.sval);
			setting->type = CONFIG_TYPE_SPECIAL;
			setting->value.sval = (char*) documentRoot;
		} break;
		case PANCAKE_CONFIGURATION_DTOR: {
			// Free memory
			documentRoot = (String*) setting->value.sval;
			PancakeFree(documentRoot->value);
			PancakeFree(documentRoot);

			// Make library happy
			setting->type = CONFIG_TYPE_NONE;
		} break;
	}

	return 1;
}

static UByte PancakeHTTPNetworkInterfaceConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	if(PancakeNetworkInterfaceConfiguration(step, setting, scope) && step == PANCAKE_CONFIGURATION_INIT) {
		PancakeSocket *sock = (PancakeSocket*) setting->hook;

		sock->onRead = PancakeHTTPInitializeConnection;

		return 1;
	}

	return 0;
}

static UByte PancakeHTTPContentServeBackendConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	config_setting_t *element;
	UInt16 i = 0;
	PancakeHTTPVirtualHost *vHost = (PancakeHTTPVirtualHost*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		while(element = config_setting_get_elem(setting, i++)) {
			PancakeHTTPContentServeBackend *backend = NULL, *tmp;

			LL_FOREACH(contentBackends, tmp) {
				if(!strcmp(tmp->name, element->value.sval)) {
					backend = tmp;
					break;
				}
			}

			if(backend == NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown HTTP content serve backend: %s", element->value.sval);
				return 0;
			}

			vHost->numContentBackends++;
			vHost->contentBackends = PancakeReallocate(vHost->contentBackends, vHost->numContentBackends * sizeof(PancakeHTTPContentServeBackend*));
			vHost->contentBackends[vHost->numContentBackends - 1] = backend->handler;
		}
	} else {
		if(vHost->contentBackends) {
			PancakeFree(vHost->contentBackends);
		}
	}

	return 1;
}

static UByte PancakeHTTPOutputFilterConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	config_setting_t *element;
	UInt16 i = 0;
	PancakeHTTPVirtualHost *vHost = (PancakeHTTPVirtualHost*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		while(element = config_setting_get_elem(setting, i++)) {
			PancakeHTTPOutputFilter *filter = NULL, *tmp;

			LL_FOREACH(outputFilters, tmp) {
				if(!strcmp(tmp->name, element->value.sval)) {
					filter = tmp;
					break;
				}
			}

			if(filter == NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown HTTP output filter: %s", element->value.sval);
				return 0;
			}

			vHost->numOutputFilters++;
			vHost->outputFilters = PancakeReallocate(vHost->outputFilters, vHost->numOutputFilters * sizeof(PancakeHTTPOutputFilter*));
			vHost->outputFilters[vHost->numOutputFilters - 1] = filter->handler;
		}
	} else {
		if(vHost->outputFilters) {
			PancakeFree(vHost->outputFilters);
		}
	}

	return 1;
}

static UByte PancakeHTTPParserHookConfiguration(UByte step, config_setting_t *setting, PancakeConfigurationScope **scope) {
	config_setting_t *element;
	UInt16 i = 0;
	PancakeHTTPVirtualHost *vHost = (PancakeHTTPVirtualHost*) setting->parent->hook;

	if(step == PANCAKE_CONFIGURATION_INIT) {
		while(element = config_setting_get_elem(setting, i++)) {
			PancakeHTTPParserHook *hook = NULL, *tmp;

			LL_FOREACH(parserHooks, tmp) {
				if(!strcmp(tmp->name, element->value.sval)) {
					hook = tmp;
					break;
				}
			}

			if(hook == NULL) {
				PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Unknown HTTP parser hook: %s", element->value.sval);
				return 0;
			}

			vHost->numParserHooks++;
			vHost->parserHooks = PancakeReallocate(vHost->parserHooks, vHost->numParserHooks * sizeof(PancakeHTTPParserHook*));
			vHost->parserHooks[vHost->numParserHooks - 1] = hook->handler;
		}
	} else {
		if(vHost->parserHooks) {
			PancakeFree(vHost->parserHooks);
		}
	}

	return 1;
}

static UByte initDeferred = 0;

UByte PancakeHTTPInitialize() {
	PancakeConfigurationGroup *group, *child;
	PancakeConfigurationSetting *setting, *serverHeader;

	if(!initDeferred) {
		// Defer once to make sure all network layers are registered before creating interface group
		initDeferred = 1;

		return 2;
	}

#ifdef PANCAKE_NETWORK_TLS
	PancakeHTTPSRegisterProtocol();
#endif

	group = PancakeConfigurationAddGroup(NULL, (String) {"HTTP", sizeof("HTTP") - 1}, NULL);
	PancakeConfigurationAddSetting(group, StaticString("RequestTimeout"), CONFIG_TYPE_INT, &PancakeHTTPConfiguration.requestTimeout, sizeof(UInt32), (config_value_t) 10, NULL);
	PancakeConfigurationAddSetting(group, StaticString("KeepAliveTimeout"), CONFIG_TYPE_INT, &PancakeHTTPConfiguration.keepAliveTimeout, sizeof(UInt32), (config_value_t) 10, NULL);
	serverHeader = PancakeConfigurationAddSetting(group, (String) {"ServerHeader", sizeof("ServerHeader") - 1}, CONFIG_TYPE_BOOL, &PancakeHTTPConfiguration.serverHeader, sizeof(UByte), (config_value_t) 0, NULL);
	PancakeNetworkRegisterListenInterfaceGroup(group, PancakeHTTPNetworkInterfaceConfiguration);

	setting = PancakeConfigurationAddSetting(group, (String) {"VirtualHosts", sizeof("VirtualHosts") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, NULL);
	group = PancakeConfigurationListGroup(setting, PancakeHTTPVirtualHostConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Hosts", sizeof("Hosts") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, PancakeHTTPHostsConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"Default", sizeof("Default") - 1}, CONFIG_TYPE_BOOL, NULL, 0, (config_value_t) 0, PancakeHTTPDefaultConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"DocumentRoot", sizeof("DocumentRoot") - 1}, CONFIG_TYPE_STRING, &PancakeHTTPConfiguration.documentRoot, sizeof(String*), (config_value_t) "", PancakeHTTPDocumentRootConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"ContentServeBackends", sizeof("ContentServeBackends") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, PancakeHTTPContentServeBackendConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"OutputFilters", sizeof("OutputFilters") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, PancakeHTTPOutputFilterConfiguration);
	PancakeConfigurationAddSetting(group, (String) {"ParserHooks", sizeof("ParserHooks") - 1}, CONFIG_TYPE_LIST, NULL, 0, (config_value_t) 0, PancakeHTTPParserHookConfiguration);

	PancakeConfigurationAddSettingToGroup(group, serverHeader);

	child = PancakeConfigurationLookupGroup(NULL, (String) {"Logging", sizeof("Logging") - 1});
	PancakeConfigurationAddGroupToGroup(group, child);

	return 1;
}

static inline void PancakeHTTPInitializeRequestStructure(PancakeHTTPRequest *request) {
	request->method = 0;
	request->headers = NULL;
	request->answerHeaders = NULL;
	request->requestAddress.value = NULL;
	request->host.value = NULL;
	request->host.length = 0;
	request->path.value = NULL;
	request->keepAlive = 0;
	request->onRequestEnd = NULL;
	request->ifModifiedSince.value = NULL;
	request->outputFilterData = NULL;
	request->onOutputEnd = NULL;
	request->acceptEncoding.length = 0;
	request->authorization.length = 0;
	request->clientContentLength = 0;
	request->schedulerEvent = NULL;

	PancakeConfigurationInitializeScopeGroup(&request->scopeGroup);
}

static void PancakeHTTPInitializeConnection(PancakeSocket *sock) {
	PancakeSocket *client = PancakeNetworkAcceptConnection(sock);
	PancakeHTTPRequest *request;

	if(client == NULL) {
		return;
	}

	request = PancakeAllocate(sizeof(PancakeHTTPRequest));
	PancakeHTTPInitializeRequestStructure(request);

	request->socket = client;

	client->onRead = PancakeHTTPReadHeaderData;
	client->onRemoteHangup = PancakeHTTPOnRemoteHangup;
	client->data = (void*) request;

	PancakeNetworkAddReadSocket(client);
	PancakeHTTPReadHeaderData(client);
}

static void PancakeHTTPInitializeKeepAliveConnection(PancakeSocket *sock) {
	PancakeHTTPRequest *request;

	request = PancakeAllocate(sizeof(PancakeHTTPRequest));
	PancakeHTTPInitializeRequestStructure(request);

	request->socket = sock;

	// Unschedule keep-alive timeout event
	PancakeUnschedule((PancakeSchedulerEvent*) sock->data);

	sock->onRead = PancakeHTTPReadHeaderData;
	sock->onRemoteHangup = PancakeHTTPOnRemoteHangup;
	sock->data = (void*) request;

	PancakeHTTPReadHeaderData(sock);
}

static inline void PancakeHTTPCleanRequestData(PancakeHTTPRequest *request) {
	PancakeHTTPHeader *header, *tmp;

	if(request->onRequestEnd) {
		request->onRequestEnd(request);
	}

	if(request->requestAddress.value) {
		PancakeFree(request->requestAddress.value);
	}

	LL_FOREACH_SAFE(request->headers, header, tmp) {
		PancakeFree(header);
	}

	PancakeConfigurationDestroyScopeGroup(&request->scopeGroup);
}

static void PancakeHTTPOnClientTimeout(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(request != NULL) {
		PancakeHTTPCleanRequestData(request);

		PancakeFree(sock->data);
	}

	PancakeNetworkClose(sock);
}

static void PancakeHTTPReadHeaderData(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	// Read data from socket
	if(PancakeNetworkRead(sock, 1536) == -1) {
		return;
	}

	// Parse HTTP
	if(EXPECTED(sock->readBuffer.length >= 5)) {
		UByte *offset, *headerEnd, *ptr, *ptr2, *ptr3;
		UInt8 i;

		if(!request->method) {
			if(sock->readBuffer.value[0] == 'G') {
				if(UNEXPECTED(sock->readBuffer.value[1] != 'E'
				|| sock->readBuffer.value[2] != 'T'
				|| sock->readBuffer.value[3] != ' ')) {
					PancakeHTTPOnRemoteHangup(sock);
					return;
				}

				request->method = PANCAKE_HTTP_GET;
			} else if(sock->readBuffer.value[0] == 'P') {
				if(UNEXPECTED(sock->readBuffer.value[1] != 'O'
				|| sock->readBuffer.value[2] != 'S'
				|| sock->readBuffer.value[3] != 'T'
				|| sock->readBuffer.value[4] != ' ')) {
					PancakeHTTPOnRemoteHangup(sock);
					return;
				}

				request->method = PANCAKE_HTTP_POST;
			} else if(sock->readBuffer.value[0] == 'H') {
				if(UNEXPECTED(sock->readBuffer.value[1] != 'E'
				|| sock->readBuffer.value[2] != 'A'
				|| sock->readBuffer.value[3] != 'D'
				|| sock->readBuffer.value[4] != ' ')) {
					PancakeHTTPOnRemoteHangup(sock);
					return;
				}

				request->method = PANCAKE_HTTP_HEAD;
			} else {
				// Probably non-HTTP data, disconnect the client
				PancakeHTTPOnRemoteHangup(sock);
				return;
			}
		}

		if(sock->readBuffer.length >= 10240) {
			// Header too large
			PancakeHTTPOnRemoteHangup(sock);
			return;
		}

		if(request->method == PANCAKE_HTTP_POST) {
			// Lookup end of header data since the end of the buffer is not necessarily \r\n\r\n
			offset = ptr = sock->readBuffer.value + 5;

			while(ptr = memchr(ptr, '\r', sock->readBuffer.value + sock->readBuffer.length - ptr)) {
				if(sock->readBuffer.value + sock->readBuffer.length - ptr < 3) {
					if(!request->schedulerEvent) {
						request->schedulerEvent = PancakeSchedule(time(NULL) + PancakeHTTPConfiguration.requestTimeout, (PancakeSchedulerEventCallback) PancakeHTTPOnClientTimeout, sock);
					}

					return;
				}

				if(ptr[1] == '\n' && ptr[2] == '\r' && ptr[3] == '\n') {
					headerEnd = ptr;
					break;
				}

				ptr++;
			}

			if(ptr == NULL) {
				if(!request->schedulerEvent) {
					request->schedulerEvent = PancakeSchedule(time(NULL) + PancakeHTTPConfiguration.requestTimeout, (PancakeSchedulerEventCallback) PancakeHTTPOnClientTimeout, sock);
				}

				return;
			}
		} else if(sock->readBuffer.value[sock->readBuffer.length - 1] == '\n'
			&& sock->readBuffer.value[sock->readBuffer.length - 2] == '\r'
			&& sock->readBuffer.value[sock->readBuffer.length - 3] == '\n'
			&& sock->readBuffer.value[sock->readBuffer.length - 4] == '\r') {
			offset = sock->readBuffer.value + (request->method == PANCAKE_HTTP_HEAD ? 5 : 4); // 5 = "HEAD "; 4 = "GET "
			headerEnd = sock->readBuffer.value + sock->readBuffer.length - 4;
		} else {
			if(!request->schedulerEvent) {
				request->schedulerEvent = PancakeSchedule(time(NULL) + PancakeHTTPConfiguration.requestTimeout, (PancakeSchedulerEventCallback) PancakeHTTPOnClientTimeout, sock);
			}

			return;
		}

		// Unschedule timeout event
		if(request->schedulerEvent) {
			PancakeUnschedule(request->schedulerEvent);
			request->schedulerEvent = NULL;
		}

		// Lookup end of request URI
		ptr = memchr(offset, ' ', headerEnd - offset);
		if(UNEXPECTED(!ptr || ptr == offset)) {
			// Malformed header
			PancakeHTTPOnRemoteHangup(sock);
			return;
		}

		request->headerEnd = headerEnd - sock->readBuffer.value;

		// Copy request URI
		request->requestAddress.length = ptr - offset;
		request->requestAddress.value = PancakeAllocate(request->requestAddress.length);
		memcpy(request->requestAddress.value, offset, request->requestAddress.length);

		// Resolve request URI
		if(*offset != '/') {
			if(headerEnd > offset + sizeof("http://") - 1 && memcmp(offset, "http://", sizeof("http://") - 1) == 0) {
				// http://abc.net[/aaa]
				offset += sizeof("http://") - 1;
				request->host.value = offset;

				if(ptr2 = memchr(offset, '/', ptr - offset)) {
					// http://abc.net/aaa
					request->host.length = ptr2 - offset;
					request->path.value = ptr2;
					request->path.length = ptr - ptr2;
				} else {
					// http://abc.net
					request->host.length = ptr - offset;
					request->path.value = offset - 1;
					request->path.length = 1;
					*(offset - 1) = '/';
				}
			} else {
				// aaa
				// offset - 1 MUST be a space, therefore we can simply overwrite it
				*(offset - 1) = '/';
				request->path.value = offset - 1;
				request->path.length = ptr - offset + 1;
			}
		} else {
			// /aaa
			request->path.value = offset;
			request->path.length = ptr - offset;
		}

		offset = ptr + 1;

		// Fetch HTTP version
		if(UNEXPECTED(offset + sizeof("HTTP/1.1") - 1 > headerEnd || memcmp(offset, "HTTP/1.", sizeof("HTTP/1.") - 1) != 0)) {
			// Malformed header
			PancakeHTTPOnRemoteHangup(sock);
			return;
		}

		offset += sizeof("HTTP/1.") - 1;

		if(*offset == '1') {
			// HTTP/1.1
			request->HTTPVersion = PANCAKE_HTTP_11;
		} else if(*offset == '0') {
			// HTTP/1.0
			request->HTTPVersion = PANCAKE_HTTP_10;
		} else {
			// Unknown HTTP version
			PancakeHTTPOnRemoteHangup(sock);
			return;
		}

		offset++;

		if(EXPECTED(offset != headerEnd)) {
			if(UNEXPECTED(*offset != '\r' || *(offset + 1) != '\n')) {
				// Malformed header
				PancakeHTTPOnRemoteHangup(sock);
				return;
			}

			offset += 2;

			// Parse header lines
			while(1) {
				PancakeHTTPHeader *header;

				// memchr() can't fail since we have a \r\n\r\n at the end of the header for sure
				ptr = memchr(offset, '\r', headerEnd - offset + 1);
				PancakeAssert(ptr != NULL);
				if(UNEXPECTED(*(ptr + 1) != '\n')) {
					// Malformed header
					PancakeHTTPOnRemoteHangup(sock);
					return;
				}

				ptr2 = memchr(offset, ':', ptr - offset);

				if(UNEXPECTED(!ptr2 || ptr2 == offset)) {
					// Malformed header
					PancakeHTTPOnRemoteHangup(sock);
					return;
				}

				// Make header name lowercase
				ptr3 = offset;
				while(ptr3 != ptr2) {
					*ptr3 = tolower(*ptr3);
					ptr3++;
				}

				// Get pointer to value
				ptr3 = ptr2 + 1;

				// RFC 2616 section 4.2 states that the colon may be followed by any amount of spaces
				while(isspace(*ptr3) && ptr3 < ptr) {
					ptr3++;
				}

				// ptr2 - offset == length of header name
				switch(ptr2 - offset) {
					case 4:
						if(!memcmp(offset, "host", 4)) {
							request->host.value = ptr3;
							request->host.length = ptr - ptr3;
							break;
						}
						goto StoreHeader;
					case 10:
						if(!memcmp(offset, "connection", 10)) {
							if((ptr - ptr3) == (sizeof("keep-alive") - 1)
								&& !strncasecmp(ptr3, "keep-alive", sizeof("keep-alive") - 1)) {
								request->keepAlive = 1;
							}

							break;
						}
						goto StoreHeader;
					case 13:
						if(!memcmp(offset, "authorization", 13)) {
							request->authorization.offset = ptr3 - sock->readBuffer.value;
							request->authorization.length = ptr - ptr3;
						}
						goto StoreHeader;
					case 14:
						if(!memcmp(offset, "content-length", 14)) {
							request->clientContentLength = atoi(ptr3);

							break;
						}
						goto StoreHeader;
					case 15:
						if(!memcmp(offset, "accept-encoding", 15)) {
							request->acceptEncoding.offset = ptr3 - sock->readBuffer.value;
							request->acceptEncoding.length = ptr - ptr3;
							break;
						}
						goto StoreHeader;
					case 17:
						if(!memcmp(offset, "if-modified-since", 17)) {
							request->ifModifiedSince.value = ptr3;
							request->ifModifiedSince.length = ptr - ptr3;
							break;
						}
						goto StoreHeader;
					default:
					StoreHeader:
						header = PancakeAllocate(sizeof(PancakeHTTPHeader));
						header->name.value = offset;
						header->name.length = ptr2 - offset;
						header->value.value = ptr3;
						header->value.length = ptr - ptr3;

						// Add header to list
						LL_APPEND(request->headers, header);
						break;
				}

				if(ptr == headerEnd) {
					// Finished parsing headers
					break;
				}

				offset = ptr + 2;
			}
		}

		// Fetch virtual host
		if(PancakeHTTPNumVirtualHosts == 1 || !request->host.value) {
			request->vHost = PancakeHTTPDefaultVirtualHost;
		} else {
			PancakeHTTPVirtualHostIndex *index;

			HASH_FIND(hh, PancakeHTTPVirtualHosts, request->host.value, request->host.length, index);

			if(index == NULL) {
				request->vHost = PancakeHTTPDefaultVirtualHost;
			} else {
				request->vHost = index->vHost;
			}
		}

		// Initialize scope group
		PancakeConfigurationScopeGroupAddScope(&request->scopeGroup, request->vHost->configurationScope);
		PancakeConfigurationActivateScope(request->vHost->configurationScope);

		// Initialize some values
		request->statDone = 0;
		request->contentLength = 0;
		request->answerType = NULL;
		request->chunkedTransfer = 0;
		request->lastModified = 0;
		request->outputFilter = NULL;
		request->contentEncoding = NULL;
		request->headerSent = 0;
		request->answerCode = 0;

		// Disable reading on socket
		PancakeNetworkSetSocket(sock);

		// Decode path
		for(offset = request->path.value; offset < request->path.value + request->path.length; offset++) {
			if(*offset == '%') {
				if(offset + 2 <= request->path.value + request->path.length) {
					if(isdigit(offset[1])) {
						*offset = (offset[1] - '0') * 16;
					} else {
						UByte lower = tolower(offset[1]);

						if(EXPECTED(lower >= 'a' && lower <= 'f')) {
							*offset = (lower - 'a' + 10) * 16;
						} else {
							// Invalid character (non-hex)
							continue;
						}
					}

					if(isdigit(offset[2])) {
						*offset += offset[2] - '0';
					} else {
						UByte lower = tolower(offset[2]);

						if(EXPECTED(lower >= 'a' && lower <= 'f')) {
							*offset += lower - 'a' + 10;
						} else {
							// Invalid second character, reset character at offset to %
							*offset = '%';
							continue;
						}
					}

					memmove(offset + 1, offset + 3, request->path.value + request->path.length - offset - 2);

					request->path.length -= 2;
				}
			} else if(*offset == '+') {
				*offset = ' ';
			} else if(*offset == '?') {
				// Stop on query string
				break;
			}
		}

		// Call parser hooks
		for(i = 0; i < request->vHost->numParserHooks; i++) {
			if(!request->vHost->parserHooks[i](sock)) {
				PancakeConfigurationUnscope();
				return;
			}
		}

		// Serve content
		if(EXPECTED(PancakeHTTPServeContent(sock, 0))) {
			PancakeConfigurationUnscope();
			return;
		}

		// No content available
		PancakeHTTPException(sock, 500);
		PancakeConfigurationUnscope();
	} else if(!request->schedulerEvent) {
		request->schedulerEvent = PancakeSchedule(time(NULL) + PancakeHTTPConfiguration.requestTimeout, (PancakeSchedulerEventCallback) PancakeHTTPOnClientTimeout, sock);
	}
}

PANCAKE_API void PancakeHTTPException(PancakeSocket *sock, UInt16 code) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
	UByte *offset;

	PancakeAssert(code >= 100 && code <= 599);
	request->answerCode = code;

	// Check for previous exception (prevent recursion)
	if(sock->flags & PANCAKE_HTTP_EXCEPTION) {
		return;
	}

	// Set exception flag
	sock->flags |= PANCAKE_HTTP_EXCEPTION;

	// Calculate page size
	request->contentLength = 8 /* answer code length + whitespace * 2 */
			+ PancakeHTTPAnswerCodes[code - 100].length * 2 /* <title> + <h1> */
			+ (sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_HEADER) - 1)
			+ (sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_BODY_ERROR) - 1)
			+ (sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_BODY_TOKEN) - 1)
			+ (PancakeHTTPConfiguration.serverHeader ? (sizeof(PANCAKE_HTTP_SERVER_TOKEN) - 1) : 0)
			+ (sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_FOOTER) - 1);

	// Set answer headers
	PancakeHTTPBuildAnswerHeaders(sock);

	// Reallocate buffer to correct size
	sock->writeBuffer.size = sock->writeBuffer.length + request->contentLength;
	sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, sock->writeBuffer.size);

	// Build exception page
	offset = sock->writeBuffer.value + sock->writeBuffer.length;
	memcpy(offset, PANCAKE_HTTP_EXCEPTION_PAGE_HEADER, sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_HEADER) - 1);
	offset += sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_HEADER) - 1;

	// answer code in <title>
	itoa(request->answerCode, offset, 10);
	offset += 3;
	*offset = ' ';
	offset++;
	memcpy(offset, PancakeHTTPAnswerCodes[code - 100].value, PancakeHTTPAnswerCodes[code - 100].length);
	offset += PancakeHTTPAnswerCodes[code - 100].length;

	memcpy(offset, PANCAKE_HTTP_EXCEPTION_PAGE_BODY_ERROR, sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_BODY_ERROR) - 1);
	offset += sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_BODY_ERROR) - 1;

	// answer code in <h1>
	itoa(request->answerCode, offset, 10);
	offset += 3;
	*offset = ' ';
	offset++;
	memcpy(offset, PancakeHTTPAnswerCodes[code - 100].value, PancakeHTTPAnswerCodes[code - 100].length);
	offset += PancakeHTTPAnswerCodes[code - 100].length;

	memcpy(offset, PANCAKE_HTTP_EXCEPTION_PAGE_BODY_TOKEN, sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_BODY_TOKEN) - 1);
	offset += sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_BODY_TOKEN) - 1;

	// Pancake server token
	if(PancakeHTTPConfiguration.serverHeader) {
		memcpy(offset, PANCAKE_HTTP_SERVER_TOKEN, sizeof(PANCAKE_HTTP_SERVER_TOKEN) - 1);
		offset += sizeof(PANCAKE_HTTP_SERVER_TOKEN) - 1;
	}

	// Footer
	memcpy(offset, PANCAKE_HTTP_EXCEPTION_PAGE_FOOTER, sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_FOOTER) - 1);
	offset += sizeof(PANCAKE_HTTP_EXCEPTION_PAGE_FOOTER) - 1;

	sock->writeBuffer.length = sock->writeBuffer.size;

	PancakeNetworkSetWriteSocket(sock);
	sock->onWrite = PancakeHTTPFullWriteBuffer;
}

PANCAKE_API inline UByte PancakeHTTPServeContent(PancakeSocket *sock, UByte ignoreException) {
	UInt16 i;
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	for(i = 0; i < request->vHost->numContentBackends; i++) {
		if(request->vHost->contentBackends[i](sock) || (!ignoreException && (sock->flags & PANCAKE_HTTP_EXCEPTION))) {
			return 1;
		}
	}

	return 0;
}

PANCAKE_API inline void PancakeHTTPFreeContentEncoding(PancakeHTTPRequest *request) {
	PancakeFree(request->contentEncoding);
}

PANCAKE_API inline void PancakeHTTPOnRemoteHangup(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(request != NULL) {
		PancakeHTTPCleanRequestData(request);

		if(request->schedulerEvent) {
			PancakeUnschedule(request->schedulerEvent);
		}

		PancakeFree(sock->data);
	}

	PancakeNetworkClose(sock);
}

static void PancakeHTTPOnKeepAliveRemoteHangup(PancakeSocket *sock) {
	PancakeUnschedule((PancakeSchedulerEvent*) sock->data);
	PancakeNetworkClose(sock);
}

PANCAKE_API inline void PancakeHTTPFullWriteBuffer(PancakeSocket *sock) {
	PancakeNetworkWrite(sock);

	if(!sock->writeBuffer.length) {
		PancakeHTTPOnRequestEnd(sock);
	}
}

PANCAKE_API inline void PancakeHTTPOnWrite(PancakeSocket *sock) {
	PancakeNetworkWrite(sock);

	if(!sock->writeBuffer.length) {
		PancakeNetworkSetSocket(sock);
	}
}

PANCAKE_API inline void PancakeHTTPRemoveQueryString(PancakeHTTPRequest *request) {
	UByte *offset = memchr(request->path.value, '?', request->path.length);

	if(offset) {
		request->path.length = offset - request->path.value;
	}
}

PANCAKE_API inline void PancakeHTTPExtractQueryString(PancakeHTTPRequest *request, String *queryString) {
	UByte *offset = memchr(request->path.value, '?', request->path.length);

	if(offset) {
		queryString->length = request->path.value + request->path.length - offset - 1;
		queryString->value = offset + 1;

		request->path.length = offset - request->path.value;
	} else {
		queryString->length = 0;
		queryString->value = NULL;
	}
}

PANCAKE_API UByte PancakeHTTPRunAccessChecks(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(!request->statDone) {
		UByte fullPath[PancakeHTTPConfiguration.documentRoot->length + request->path.length + 1];
		UByte *offset, *dot;

		// Find query string offset
		offset = memchr(request->path.value, '?', request->path.length);

		memcpy(fullPath, PancakeHTTPConfiguration.documentRoot->value, PancakeHTTPConfiguration.documentRoot->length);
		memcpy(fullPath + PancakeHTTPConfiguration.documentRoot->length, request->path.value, offset ? offset - request->path.value : request->path.length);

		// null-terminate string
		fullPath[PancakeHTTPConfiguration.documentRoot->length + (offset ? offset - request->path.value : request->path.length)] = '\0';

		// Try to stat file
		if(stat(fullPath, &request->fileStat) == -1) {
			PancakeHTTPException(sock, 404);
			return 0;
		}

		offset = fullPath;

		// Disallow requests to paths lower than the document root
		while(dot = memchr(offset, '.', fullPath + PancakeHTTPConfiguration.documentRoot->length + request->path.length + 1 - offset)) {
			if(dot[1] == '.') {
				// .. detected, possible exploit
				UByte resolvedPath[PATH_MAX];

				if(realpath(fullPath, resolvedPath) == NULL
				|| strlen(resolvedPath) < PancakeHTTPConfiguration.documentRoot->length
				|| memcmp(fullPath, resolvedPath, PancakeHTTPConfiguration.documentRoot->length)) {
					PancakeHTTPException(sock, 403);
					return 0;
				}

				break;
			}

			offset = dot + 1;
		}
	}

	if(!S_ISREG(request->fileStat.st_mode) && !S_ISDIR(request->fileStat.st_mode)) {
		return 0;
	}

	return 1;
}

PANCAKE_API void PancakeHTTPOutput(PancakeSocket *sock, String *output) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
	UInt16 i;

	// Run output filter if available
	if(request->outputFilter) {
		request->outputFilter(sock, output);
		return;
	} else for(i = 0; i < request->vHost->numOutputFilters; i++) {
		if(request->vHost->outputFilters[i](sock, output)) {
			request->outputFilter = request->vHost->outputFilters[i];
			return;
		}
	}

	// No output filter available
	// Build answer headers
	if(!request->headerSent) {
		PancakeHTTPBuildAnswerHeaders(sock);
	}

	// Resize buffer
	if(sock->writeBuffer.size < sock->writeBuffer.length + output->length) {
		sock->writeBuffer.size += output->length + 2048;
		sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, sock->writeBuffer.size);
	}

	// Copy to buffer
	memcpy(sock->writeBuffer.value + sock->writeBuffer.length, output->value, output->length);
	sock->writeBuffer.length += output->length;
}

PANCAKE_API void PancakeHTTPOutputChunk(PancakeSocket *sock, String *output) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
	UInt16 i;

	// Run output filter if available
	if(request->outputFilter) {
		request->outputFilter(sock, output);
		return;
	} else for(i = 0; i < request->vHost->numOutputFilters; i++) {
		if(request->vHost->outputFilters[i](sock, output)) {
			request->outputFilter = request->vHost->outputFilters[i];
			return;
		}
	}

	// Send chunk if no output filter available
	PancakeHTTPSendChunk(sock, output);
}

PANCAKE_API void PancakeHTTPSendChunk(PancakeSocket *sock, String *chunk) {
	UByte *offset;
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	// Do not use HTTP chunking on HTTP 1.0 connections
	if(request->HTTPVersion == PANCAKE_HTTP_10) {
		PancakeAssert(request->headerSent == 0);

		if(sock->writeBuffer.size < sock->writeBuffer.length + chunk->length) {
			sock->writeBuffer.size = sock->writeBuffer.length + chunk->length;
			sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, sock->writeBuffer.size);
		}

		memcpy(sock->writeBuffer.value + sock->writeBuffer.length, chunk->value, chunk->length);
		sock->writeBuffer.length += chunk->length;
		return;
	}

	// Send headers if not done yet
	if(!request->headerSent) {
		PancakeHTTPBuildAnswerHeaders(sock);
	}

	// Reallocate buffer if necessary
	if(sock->writeBuffer.size < sock->writeBuffer.length + chunk->length + sizeof("ffffffff\r\n\r\n") - 1) {
		sock->writeBuffer.size = sock->writeBuffer.length + chunk->length + sizeof("ffffffff\r\n\r\n") - 1;
		sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, sock->writeBuffer.size);
	}

	// Write length as hex value
	offset = sock->writeBuffer.value + sock->writeBuffer.length;
	offset += sprintf(offset, "%lx", chunk->length);

	offset[0] = '\r';
	offset[1] = '\n';
	offset += 2;

	// Copy chunk into buffer
	memcpy(offset, chunk->value, chunk->length);
	offset += chunk->length;

	offset[0] = '\r';
	offset[1] = '\n';

	// Set buffer length
	sock->writeBuffer.length = offset + 2 - sock->writeBuffer.value;
}

PANCAKE_API inline void PancakeHTTPSendLastChunk(PancakeSocket *sock) {
	UByte *offset;

	if(sock->writeBuffer.size < sock->writeBuffer.length + sizeof("0\r\n\r\n") - 1) {
		sock->writeBuffer.size += sizeof("0\r\n\r\n") - 1;
		sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, sock->writeBuffer.size);
	}

	offset = sock->writeBuffer.value + sock->writeBuffer.length;
	offset[0] = '0';
	offset[1] = '\r';
	offset[2] = '\n';
	offset[3] = '\r';
	offset[4] = '\n';

	sock->writeBuffer.length += sizeof("0\r\n\r\n") - 1;
}

PANCAKE_API void PancakeHTTPBuildAnswerHeaders(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;
	UByte *offset, alignOutput = 0;
	UInt16 headerSize = 4096;

	PancakeAssert(request->headerSent == 0);

	// Set headerSent flag
	request->headerSent = 1;

	sock->writeBuffer.size += headerSize;
	sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, sock->writeBuffer.size);

	if(request->HTTPVersion == PANCAKE_HTTP_10 && request->chunkedTransfer == 1) {
		// Chunks from content backend complete, make HTTP1.0-compatible transfer
		request->chunkedTransfer = 0;

		request->contentLength = sock->writeBuffer.length;
		memmove(sock->writeBuffer.value + headerSize, sock->writeBuffer.value, request->contentLength);

		alignOutput = 1;
	}

	// HTTP/1.x
	sock->writeBuffer.value[0] = 'H';
	sock->writeBuffer.value[1] = 'T';
	sock->writeBuffer.value[2] = 'T';
	sock->writeBuffer.value[3] = 'P';
	sock->writeBuffer.value[4] = '/';
	sock->writeBuffer.value[5] = '1';
	sock->writeBuffer.value[6] = '.';
	sock->writeBuffer.value[7] = request->HTTPVersion == PANCAKE_HTTP_11 ? '1' : '0';
	sock->writeBuffer.value[8] = ' ';

	// Answer code
	PancakeAssert(request->answerCode >= 100 && request->answerCode <= 599);
	itoa(request->answerCode, &sock->writeBuffer.value[9], 10);

	// Answer code string
	sock->writeBuffer.value[12] = ' ';
	memcpy(&sock->writeBuffer.value[13], PancakeHTTPAnswerCodes[request->answerCode - 100].value, PancakeHTTPAnswerCodes[request->answerCode - 100].length);

	// \r\n - use offsets from here on to allow for dynamic-length answer code descriptions
	offset = sock->writeBuffer.value + 13 + PancakeHTTPAnswerCodes[request->answerCode - 100].length;
	offset[0] = '\r';
	offset[1] = '\n';
	offset += 2;

	// Server
	if(PancakeHTTPConfiguration.serverHeader) {
		memcpy(offset, PANCAKE_HTTP_SERVER_HEADER, sizeof(PANCAKE_HTTP_SERVER_HEADER) - 1);
		offset += sizeof(PANCAKE_HTTP_SERVER_HEADER) - 1;
	}

	// Date
	memcpy(offset, "Date", sizeof("Date") - 1);
	offset += sizeof("Date") - 1;
	offset[0] = ':';
	offset[1] = ' ';
	offset += 2;
	PancakeRFC1123CurrentDate(offset);
	offset += 29;

	// \r\n
	offset[0] = '\r';
	offset[1] = '\n';
	offset += 2;

	// Content-Length
	if(request->contentLength && !request->chunkedTransfer) {
		memcpy(offset, "Content-Length", sizeof("Content-Length") - 1);
		offset += sizeof("Content-Length") - 1;
		offset[0] = ':';
		offset[1] = ' ';
		offset += 2;
		offset += sprintf(offset, "%i", request->contentLength);

		// \r\n
		offset[0] = '\r';
		offset[1] = '\n';
		offset += 2;
	}

	// Content-Type
	if(request->answerType) {
		memcpy(offset, "Content-Type", sizeof("Content-Type") - 1);
		offset += sizeof("Content-Type") - 1;
		offset[0] = ':';
		offset[1] = ' ';
		offset += 2;
		memcpy(offset, request->answerType->type.value, request->answerType->type.length);
		offset += request->answerType->type.length;

		// \r\n
		offset[0] = '\r';
		offset[1] = '\n';
		offset += 2;
	}

	// Last-Modified
	if(request->lastModified) {
		memcpy(offset, "Last-Modified", sizeof("Last-Modified") - 1);
		offset += sizeof("Last-Modified") - 1;
		offset[0] = ':';
		offset[1] = ' ';
		PancakeRFC1123Date(request->lastModified, offset + 2);
		offset[31] = '\r';
		offset[32] = '\n';
		offset += 33;
	}

	// Connection
	if(request->keepAlive && !PancakeDoShutdown) {
		memcpy(offset, "Connection: keep-alive\r\n", sizeof("Connection: keep-alive\r\n") - 1);
		offset += sizeof("Connection: keep-alive\r\n") - 1;
	} else {
		memcpy(offset, "Connection: close\r\n", sizeof("Connection: close\r\n") - 1);
		offset += sizeof("Connection: close\r\n") - 1;
		request->keepAlive = 0;
	}

	// Transfer-Encoding
	if(request->chunkedTransfer) {
		memcpy(offset, "Transfer-Encoding: chunked\r\n", sizeof("Transfer-Encoding: chunked\r\n") - 1);
		offset += sizeof("Transfer-Encoding: chunked\r\n") - 1;
	}

	// Content-Encoding
	if(request->contentEncoding) {
		memcpy(offset, "Content-Encoding", sizeof("Content-Encoding") - 1);
		offset += sizeof("Content-Encoding") - 1;
		offset[0] = ':';
		offset[1] = ' ';
		offset += 2;
		memcpy(offset, request->contentEncoding->value, request->contentEncoding->length);
		offset += request->contentEncoding->length;

		// \r\n
		offset[0] = '\r';
		offset[1] = '\n';
		offset += 2;
	}

	// Process custom answer headers
	if(request->answerHeaders) {
		PancakeHTTPHeader *header = NULL, *tmp;

		LL_FOREACH_SAFE(request->answerHeaders, header, tmp) {
			if((offset - sock->writeBuffer.value + header->name.length + header->value.length + 2) > headerSize) {
				sock->writeBuffer.size += 1024;
				sock->writeBuffer.value = PancakeReallocate(sock->writeBuffer.value, sock->writeBuffer.size);

				if(alignOutput) {
					memmove(sock->writeBuffer.value + headerSize + 1024, sock->writeBuffer.value + headerSize, request->contentLength);
				}

				headerSize += 1024;
			}

			memcpy(offset, header->name.value, header->name.length);
			offset += header->name.length;
			offset[0] = ':';
			offset[1] = ' ';
			offset += 2;
			memcpy(offset, header->value.value, header->value.length);
			offset += header->value.length;

			// \r\n
			offset[0] = '\r';
			offset[1] = '\n';
			offset += 2;

			// Free header instance
			PancakeFree(header->name.value);
			PancakeFree(header->value.value);
			PancakeFree(header);
		}
	}

	// \r\n
	offset[0] = '\r';
	offset[1] = '\n';

	sock->writeBuffer.length = offset - sock->writeBuffer.value + 2;

	if(alignOutput) {
		memmove(sock->writeBuffer.value + sock->writeBuffer.length, sock->writeBuffer.value + headerSize, request->contentLength);
		sock->writeBuffer.length += request->contentLength;
	}
}

PANCAKE_API inline void PancakeHTTPOnRequestEnd(PancakeSocket *sock) {
	PancakeHTTPRequest *request = (PancakeHTTPRequest*) sock->data;

	if(request->onOutputEnd) {
		request->onOutputEnd(request);
		request->onOutputEnd = NULL;
	}

	if(request->chunkedTransfer == 1 && request->method != PANCAKE_HTTP_HEAD) {
		request->chunkedTransfer = 0;
		PancakeHTTPSendLastChunk(sock);

		PancakeNetworkSetWriteSocket(sock);
		sock->onWrite = PancakeHTTPFullWriteBuffer;
		PancakeHTTPFullWriteBuffer(sock);
		return;
	}

	if(request->keepAlive) {
		PancakeNetworkSetReadSocket(sock);

		PancakeHTTPCleanRequestData(request);

		PancakeFree(sock->data);

		// Schedule keep-alive timeout event
		sock->data = (void*) PancakeSchedule(time(NULL) + PancakeHTTPConfiguration.keepAliveTimeout, (PancakeSchedulerEventCallback) PancakeNetworkClose, sock);

		// Destroy write buffer
		if(sock->writeBuffer.size) {
			sock->writeBuffer.size = 0;
			PancakeFree(sock->writeBuffer.value);
			sock->writeBuffer.value = NULL;
		}

		// Reset HTTP exception flag
		if(sock->flags & PANCAKE_HTTP_EXCEPTION) {
			sock->flags ^= PANCAKE_HTTP_EXCEPTION;
		}

		// Reset HTTP header data complete flag
		if(sock->flags & PANCAKE_HTTP_HEADER_DATA_COMPLETE) {
			sock->flags ^= PANCAKE_HTTP_HEADER_DATA_COMPLETE;
		}

		sock->readBuffer.length = 0;

		sock->onRead = PancakeHTTPInitializeKeepAliveConnection;
		sock->onRemoteHangup = PancakeHTTPOnKeepAliveRemoteHangup;
	} else {
		PancakeHTTPOnRemoteHangup(sock);
	}
}
#endif
